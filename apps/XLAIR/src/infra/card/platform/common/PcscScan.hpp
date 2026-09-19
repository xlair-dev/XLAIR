#pragma once

#include "app/interfaces/ICardReader.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace xlair::infra::card::detail {
    using ScanResult = app::card::Result;

    [[nodiscard]]
    inline ScanResult MakePcscError(const app::card::ErrorKind kind, String message) {
        return app::card::Error{
            .kind = kind,
            .message = std::move(message),
        };
    }

    template <class PcscApi> class PcscContext {
    public:
        ~PcscContext() {
            if (m_handle != decltype(m_handle){}) {
                PcscApi::ReleaseContext(m_handle);
            }
        }

        [[nodiscard]]
        typename PcscApi::Context* put() noexcept {
            return &m_handle;
        }

        [[nodiscard]]
        typename PcscApi::Context get() const noexcept {
            return m_handle;
        }

    private:
        typename PcscApi::Context m_handle{};
    };

    template <class PcscApi> class PcscCard {
    public:
        ~PcscCard() {
            if (m_handle != decltype(m_handle){}) {
                PcscApi::Disconnect(m_handle);
            }
        }

        [[nodiscard]]
        typename PcscApi::Card* put() noexcept {
            return &m_handle;
        }

        [[nodiscard]]
        typename PcscApi::Card get() const noexcept {
            return m_handle;
        }

    private:
        typename PcscApi::Card m_handle{};
    };

    template <class PcscApi>
    [[nodiscard]]
    Array<String> SplitMultiString(const typename PcscApi::NativeChar* value) {
        Array<String> result;
        if (!value) {
            return result;
        }

        for (const auto* entry = value; *entry != 0; entry += (PcscApi::Length(entry) + 1)) {
            result.push_back(PcscApi::Decode(entry));
        }
        return result;
    }

    [[nodiscard]]
    inline Optional<String> SelectPaSoRiReader(const Array<String>& readers) {
        const auto found = std::find_if(readers.begin(), readers.end(), [](const String& reader) {
            const String normalized = reader.lowercased();
            return normalized.includes(U"sony") || normalized.includes(U"pasori") || normalized.includes(U"rc-s3");
        });
        return found == readers.end() ? none : Optional<String>{ *found };
    }

    template <class PcscApi>
    [[nodiscard]]
    bool Transmit(
        const typename PcscApi::Card card,
        const typename PcscApi::IoRequest* protocol,
        const std::span<const uint8> command,
        uint8* response,
        typename PcscApi::Size& response_size
    ) {
        return PcscApi::Succeeded(
            PcscApi::Transmit(
                card,
                protocol,
                command.data(),
                static_cast<typename PcscApi::Size>(command.size()),
                response,
                &response_size
            )
        );
    }

    template <class Size>
    [[nodiscard]]
    bool HasSuccessStatus(const uint8* response, const Size response_size) {
        return response_size >= 2 && response[response_size - 2] == 0x90 && response[response_size - 1] == 0x00;
    }

    [[nodiscard]]
    inline String ToHex(const std::array<uint8, 8>& idm) {
        String result;
        result.reserve(16);
        for (const auto byte : idm) {
            result += U"{:02X}"_fmt(byte);
        }
        return result;
    }

    template <class PcscApi>
    [[nodiscard]]
    ScanResult ReadPcscCard(const std::atomic_bool& cancelled) {
        PcscContext<PcscApi> context;
        if (!PcscApi::Succeeded(PcscApi::EstablishContext(context.put()))) {
            return MakePcscError(app::card::ErrorKind::Unavailable, U"Failed to initialize PC/SC.");
        }

        typename PcscApi::Size reader_list_size = 0;
        auto status = PcscApi::ListReaders(context.get(), nullptr, &reader_list_size);
        if (!PcscApi::Succeeded(status) || reader_list_size <= 2) {
            return MakePcscError(app::card::ErrorKind::ReaderNotFound, U"No PC/SC card reader is available.");
        }

        std::vector<typename PcscApi::NativeChar> reader_list(reader_list_size);
        status = PcscApi::ListReaders(context.get(), reader_list.data(), &reader_list_size);
        if (!PcscApi::Succeeded(status)) {
            return MakePcscError(app::card::ErrorKind::Communication, U"Failed to enumerate PC/SC card readers.");
        }

        const auto reader = SelectPaSoRiReader(SplitMultiString<PcscApi>(reader_list.data()));
        if (!reader) {
            return MakePcscError(app::card::ErrorKind::ReaderNotFound, U"A supported PaSoRi reader was not found.");
        }

        const typename PcscApi::NativeString native_reader = PcscApi::Encode(*reader);
        typename PcscApi::ReaderState reader_state{};
        reader_state.szReader = native_reader.c_str();
        reader_state.dwCurrentState = PcscApi::UnawareState();

        while (!cancelled.load(std::memory_order_acquire)) {
            status = PcscApi::GetStatusChange(context.get(), &reader_state);
            if (PcscApi::TimedOut(status)) {
                continue;
            }
            if (!PcscApi::Succeeded(status)) {
                return MakePcscError(app::card::ErrorKind::Communication, U"Failed while waiting for a card.");
            }

            const bool present = PcscApi::IsCardPresent(reader_state);
            reader_state.dwCurrentState = reader_state.dwEventState;
            if (!present) {
                continue;
            }

            PcscCard<PcscApi> card;
            typename PcscApi::Protocol protocol{};
            status = PcscApi::Connect(context.get(), native_reader.c_str(), card.put(), &protocol);
            if (!PcscApi::Succeeded(status)) {
                return MakePcscError(app::card::ErrorKind::Communication, U"Failed to connect to the detected card.");
            }

            const auto* protocol_info = PcscApi::ProtocolInfo(protocol);
            constexpr std::array<uint8, 5> type_command{ 0xFF, 0xCA, 0xF3, 0x00, 0x00 };
            std::array<uint8, 258> type_response{};
            auto type_response_size = static_cast<typename PcscApi::Size>(type_response.size());
            if (Transmit<PcscApi>(card.get(), protocol_info, type_command, type_response.data(), type_response_size) &&
                HasSuccessStatus(type_response.data(), type_response_size) &&
                (type_response_size < 3 || type_response.front() != 0x04)) {
                return MakePcscError(app::card::ErrorKind::UnsupportedCard, U"The detected card is not a FeliCa card.");
            }

            constexpr std::array<uint8, 5> idm_command{ 0xFF, 0xCA, 0x00, 0x00, 0x00 };
            std::array<uint8, 258> response{};
            auto response_size = static_cast<typename PcscApi::Size>(response.size());
            if (!Transmit<PcscApi>(card.get(), protocol_info, idm_command, response.data(), response_size)) {
                return MakePcscError(app::card::ErrorKind::Communication, U"Failed to read the card IDm.");
            }
            if (!HasSuccessStatus(response.data(), response_size) || response_size < 10) {
                return MakePcscError(
                    app::card::ErrorKind::UnsupportedCard,
                    U"The detected card does not provide an IDm."
                );
            }

            std::array<uint8, 8> idm{};
            std::copy_n(response.begin(), idm.size(), idm.begin());
            return app::card::Card{ ToHex(idm) };
        }

        return MakePcscError(app::card::ErrorKind::Cancelled, U"Card scan cancelled.");
    }

    template <class PcscApi> class PcscScan final : public app::card::IScan {
    public:
        PcscScan() {
            m_task = Async([cancelled = &m_cancelled]() {
                try {
                    return ReadPcscCard<PcscApi>(*cancelled);
                } catch (...) {
                    return MakePcscError(
                        app::card::ErrorKind::Communication,
                        U"An unexpected card reader error occurred."
                    );
                }
            });
        }

        ~PcscScan() override {
            cancel();
        }

        void update() override {
            if (m_result) {
                return;
            }
            if (m_task.isReady()) {
                m_result = m_task.get();
            }
        }

        void cancel() override {
            if (m_result) {
                return;
            }
            m_cancelled.store(true, std::memory_order_release);
            if (m_task.isValid()) {
                m_task.wait();
            }
            m_result = MakePcscError(app::card::ErrorKind::Cancelled, U"Card scan cancelled.");
        }

        [[nodiscard]]
        const Optional<app::card::Result>& result() const noexcept override {
            return m_result;
        }

    private:
        std::atomic_bool m_cancelled{ false };
        AsyncTask<ScanResult> m_task;
        Optional<app::card::Result> m_result;
    };

    template <class PcscApi>
    [[nodiscard]]
    app::card::ScanRequest MakePcscScan() {
        return std::make_unique<PcscScan<PcscApi>>();
    }
}
