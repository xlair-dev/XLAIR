#include "infra/card/PasoriRCS3xx.hpp"

#include <PCSC/winscard.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <span>
#include <utility>
#include <vector>

namespace xlair::infra::card {
    namespace {
        using ScanResult = app::card::Result;

        [[nodiscard]]
        ScanResult Error(const app::card::ErrorKind kind, String message) {
            return app::card::Error{
                .kind = kind,
                .message = std::move(message),
            };
        }

        class PcscContext {
        public:
            ~PcscContext() {
                if (m_handle != 0) {
                    SCardReleaseContext(m_handle);
                }
            }

            [[nodiscard]]
            SCARDCONTEXT* put() noexcept {
                return &m_handle;
            }

            [[nodiscard]]
            SCARDCONTEXT get() const noexcept {
                return m_handle;
            }

        private:
            SCARDCONTEXT m_handle = 0;
        };

        class PcscCard {
        public:
            ~PcscCard() {
                if (m_handle != 0) {
                    SCardDisconnect(m_handle, SCARD_LEAVE_CARD);
                }
            }

            [[nodiscard]]
            SCARDHANDLE* put() noexcept {
                return &m_handle;
            }

            [[nodiscard]]
            SCARDHANDLE get() const noexcept {
                return m_handle;
            }

        private:
            SCARDHANDLE m_handle = 0;
        };

        [[nodiscard]]
        Array<String> SplitMultiString(const char* value) {
            Array<String> result;
            if (!value) {
                return result;
            }

            for (const char* entry = value; *entry != '\0'; entry += (std::strlen(entry) + 1)) {
                result.push_back(Unicode::FromUTF8(entry));
            }
            return result;
        }

        [[nodiscard]]
        Optional<String> SelectReader(const Array<String>& readers) {
            const auto found = std::find_if(readers.begin(), readers.end(), [](const String& reader) {
                const String normalized = reader.lowercased();
                return normalized.includes(U"sony") || normalized.includes(U"pasori") || normalized.includes(U"rc-s3");
            });
            return found == readers.end() ? none : Optional<String>{ *found };
        }

        [[nodiscard]]
        bool Transmit(
            const SCARDHANDLE card,
            const SCARD_IO_REQUEST* protocol,
            const std::span<const uint8> command,
            uint8* response,
            uint32_t& response_size
        ) {
            return SCardTransmit(
                       card,
                       protocol,
                       command.data(),
                       static_cast<uint32_t>(command.size()),
                       nullptr,
                       response,
                       &response_size
                   ) == SCARD_S_SUCCESS;
        }

        [[nodiscard]]
        bool HasSuccessStatus(const uint8* response, const uint32_t response_size) {
            return response_size >= 2 && response[response_size - 2] == 0x90 && response[response_size - 1] == 0x00;
        }

        [[nodiscard]]
        String ToHex(const std::array<uint8, 8>& idm) {
            String result;
            result.reserve(16);
            for (const auto byte : idm) {
                result += U"{:02X}"_fmt(byte);
            }
            return result;
        }

        [[nodiscard]]
        ScanResult ReadCard(const std::atomic_bool& cancelled) {
            PcscContext context;
            if (SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, context.put()) != SCARD_S_SUCCESS) {
                return Error(app::card::ErrorKind::Unavailable, U"Failed to initialize PC/SC.");
            }

            uint32_t reader_list_size = 0;
            auto status = SCardListReaders(context.get(), nullptr, nullptr, &reader_list_size);
            if (status != SCARD_S_SUCCESS || reader_list_size <= 2) {
                return Error(app::card::ErrorKind::ReaderNotFound, U"No PC/SC card reader is available.");
            }

            std::vector<char> reader_list(reader_list_size);
            status = SCardListReaders(context.get(), nullptr, reader_list.data(), &reader_list_size);
            if (status != SCARD_S_SUCCESS) {
                return Error(app::card::ErrorKind::Communication, U"Failed to enumerate PC/SC card readers.");
            }

            const auto reader = SelectReader(SplitMultiString(reader_list.data()));
            if (!reader) {
                return Error(app::card::ErrorKind::ReaderNotFound, U"A supported PaSoRi reader was not found.");
            }

            const std::string native_reader = reader->toUTF8();
            SCARD_READERSTATE reader_state{};
            reader_state.szReader = native_reader.c_str();
            reader_state.dwCurrentState = SCARD_STATE_UNAWARE;

            while (!cancelled.load(std::memory_order_acquire)) {
                status = SCardGetStatusChange(context.get(), 200, &reader_state, 1);
                if (status == SCARD_E_TIMEOUT) {
                    continue;
                }
                if (status != SCARD_S_SUCCESS) {
                    return Error(app::card::ErrorKind::Communication, U"Failed while waiting for a card.");
                }

                const bool present = (reader_state.dwEventState & SCARD_STATE_PRESENT) != 0;
                reader_state.dwCurrentState = reader_state.dwEventState;
                if (!present) {
                    continue;
                }

                PcscCard card;
                uint32_t protocol = 0;
                status = SCardConnect(
                    context.get(),
                    native_reader.c_str(),
                    SCARD_SHARE_SHARED,
                    SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
                    card.put(),
                    &protocol
                );
                if (status != SCARD_S_SUCCESS) {
                    return Error(app::card::ErrorKind::Communication, U"Failed to connect to the detected card.");
                }

                const SCARD_IO_REQUEST* protocol_info = protocol == SCARD_PROTOCOL_T0 ? SCARD_PCI_T0 : SCARD_PCI_T1;
                constexpr std::array<uint8, 5> type_command{ 0xFF, 0xCA, 0xF3, 0x00, 0x00 };
                std::array<uint8, 258> type_response{};
                uint32_t type_response_size = static_cast<uint32_t>(type_response.size());
                if (Transmit(card.get(), protocol_info, type_command, type_response.data(), type_response_size) &&
                    HasSuccessStatus(type_response.data(), type_response_size) &&
                    (type_response_size < 3 || type_response.front() != 0x04)) {
                    return Error(app::card::ErrorKind::UnsupportedCard, U"The detected card is not a FeliCa card.");
                }

                constexpr std::array<uint8, 5> command{ 0xFF, 0xCA, 0x00, 0x00, 0x00 };
                std::array<uint8, 258> response{};
                uint32_t response_size = static_cast<uint32_t>(response.size());
                if (!Transmit(card.get(), protocol_info, command, response.data(), response_size)) {
                    return Error(app::card::ErrorKind::Communication, U"Failed to read the card IDm.");
                }
                if (!HasSuccessStatus(response.data(), response_size) || response_size < 10) {
                    return Error(app::card::ErrorKind::UnsupportedCard, U"The detected card does not provide an IDm.");
                }

                std::array<uint8, 8> idm{};
                std::copy_n(response.begin(), idm.size(), idm.begin());
                return app::card::Card{ ToHex(idm) };
            }

            return Error(app::card::ErrorKind::Cancelled, U"Card scan cancelled.");
        }

        class PcscScan final : public app::card::IScan {
        public:
            PcscScan() {
                m_task = Async([cancelled = &m_cancelled]() {
                    try {
                        return ReadCard(*cancelled);
                    } catch (...) {
                        return Error(app::card::ErrorKind::Communication, U"An unexpected card reader error occurred.");
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
                m_result = Error(app::card::ErrorKind::Cancelled, U"Card scan cancelled.");
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
    }

    app::card::ScanRequest PasoriRCS3xx::scan() {
        return std::make_unique<PcscScan>();
    }
}
