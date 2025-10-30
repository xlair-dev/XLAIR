#include "PasoriRCS3xx.hpp"
#define NOMINMAX
#include <Windows.h>
#include <winscard.h>
#include <vector>
#include <span>
#pragma comment(lib, "winscard.lib")

namespace infra::card {

    namespace {
        // MultiSZ -> Array<String>
        static s3d::Array<s3d::String> SplitMultiString(const wchar_t* msz) {
            s3d::Array<s3d::String> out;
            if (not msz) {
                return out;
            }

            const wchar_t* p = msz;
            while (*p) {
                out << s3d::Unicode::FromWstring(p); // UTF-16 to UTF-32
                p += (wcslen(p) + 1);
            }
            return out;
        }

        // Select PaSoRi / RC-S3xx
        static size_t ChooseReaderIndex(const s3d::Array<s3d::String>& readers) {
            for (size_t i = 0; i < readers.size(); ++i) {
                const auto& r = readers[i];
                if (r.includes(U"Sony") or r.includes(U"PaSoRi") or r.includes(U"RC-S3")) {
                    return i;
                }
            }
            return 0;
        }

        // PC/SC helper
        static bool Tx(SCARDHANDLE hCard, const SCARD_IO_REQUEST* pci, std::span<const uint8> cmd, uint8* recv, DWORD& recvLen) {
            const LONG rv = SCardTransmit(hCard, pci, cmd.data(), static_cast<DWORD>(cmd.size()), nullptr, recv, &recvLen);
            return (rv == SCARD_S_SUCCESS);
        }

        // SW1 SW2 == 90 00 ?
        static bool IsOKSW(const uint8* buf, DWORD len) {
            return (len >= 2 and buf[len - 2] == 0x90 and buf[len - 1] == 0x00);
        }

        static String ToHex(const PasoriRCS3xx::IDm& idm) {
            return U"{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}"_fmt(idm[0], idm[1], idm[2], idm[3], idm[4], idm[5], idm[6], idm[7]);
        }
    }

    PasoriRCS3xx::~PasoriRCS3xx() {
        stopScan();
    }

    void PasoriRCS3xx::startScan() {
        stopScan();

        m_ready.store(false, std::memory_order_release);
        m_ok.store(true, std::memory_order_release);
        m_running.store(true, std::memory_order_release);

        m_task = AsyncTask(Read, this);
    }

    void PasoriRCS3xx::stopScan() {
        if (m_running.exchange(false)) {
            if (m_task.isValid()) {
                m_task.wait();
            }
        }
    }

    bool PasoriRCS3xx::isReady() const {
        return m_ready.load(std::memory_order_acquire);
    }

    bool PasoriRCS3xx::isOK() const {
        return m_ok.load(std::memory_order_acquire);
    }

    String PasoriRCS3xx::getID() const {
        std::scoped_lock lock{ m_mutex };
        return ToHex(m_idm);
    }

    void PasoriRCS3xx::Read(PasoriRCS3xx* self) {
        // Step 0: Initialize PC/SC
        SCARDCONTEXT hCtx{};
        if (SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, &hCtx) != SCARD_S_SUCCESS) {
            self->m_ok.store(false, std::memory_order_release);
            self->m_running.store(false, std::memory_order_release);
            return;
        }

        // Step 1: Enumrate readers
        DWORD mszLen = 0;
        LONG rv = SCardListReadersW(hCtx, nullptr, nullptr, &mszLen);
        if (rv != SCARD_S_SUCCESS || mszLen <= 2) {
            self->m_ok.store(false, std::memory_order_release);
            SCardReleaseContext(hCtx);
            self->m_running.store(false, std::memory_order_release);
            return;
        }

        std::vector<wchar_t> msz(mszLen);
        rv = SCardListReadersW(hCtx, nullptr, msz.data(), &mszLen);
        if (rv != SCARD_S_SUCCESS) {
            self->m_ok.store(false, std::memory_order_release);
            SCardReleaseContext(hCtx);
            self->m_running.store(false, std::memory_order_release);
            return;
        }

        auto readers = SplitMultiString(msz.data());
        if (readers.isEmpty()) {
            self->m_ok.store(false, std::memory_order_release);
            SCardReleaseContext(hCtx);
            self->m_running.store(false, std::memory_order_release);
            return;
        }
        const std::wstring readerW = readers[ChooseReaderIndex(readers)].toWstr();

        // Step 2: Loop
        SCARD_READERSTATEW st{};
        st.szReader = readerW.c_str();
        st.dwCurrentState = 0; // SCARD_STATE_UNAWARE

        while (self->m_running.load(std::memory_order_acquire)) {
            // Step 2.0: 200ms timeout
            if (SCardGetStatusChangeW(hCtx, 200, &st, 1) != SCARD_S_SUCCESS) {
                continue;
            }
            const bool present = (st.dwEventState & SCARD_STATE_PRESENT) != 0;
            st.dwCurrentState = st.dwEventState;
            if (!present) {
                continue;
            }

            // Step 2.1: Connect
            SCARDHANDLE hCard{};
            DWORD proto{};
            rv = SCardConnectW(hCtx, readerW.c_str(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &proto);
            if (rv != SCARD_S_SUCCESS) {
                continue;
            }
            const SCARD_IO_REQUEST* pci = (proto == SCARD_PROTOCOL_T0) ? SCARD_PCI_T0 : SCARD_PCI_T1;

            // Step 2.2: Check card
            // FF CA F3 00 00 -> [type] 90 00  （FeliCa = 0x04）
            std::array<uint8, 258> rType{};
            DWORD nType = static_cast<DWORD>(rType.size());
            static constexpr uint8 cmdType[] = { 0xFF, 0xCA, 0xF3, 0x00, 0x00 };
            bool isFelica = false;
            if (Tx(hCard, pci, std::span<const uint8>(cmdType, sizeof(cmdType)), rType.data(), nType) && IsOKSW(rType.data(), nType)) {
                if (nType >= 3 and rType[0] == 0x04) {
                    isFelica = true;
                }
            } else {
                // 環境により未実装の場合があるため、ここでは緩く続行
                isFelica = true;
            }

            PasoriRCS3xx::IDm got{};
            bool ok = false;

            if (isFelica) {
                // Step 2.3: Get IDm
                // FF CA 00 00 00 → [idm(8)] 90 00
                std::array<uint8, 258> rIDm{}; DWORD nIDm = static_cast<DWORD>(rIDm.size());
                static constexpr uint8 cmdIDm[] = { 0xFF, 0xCA, 0x00, 0x00, 0x00 };
                if (Tx(hCard, pci, std::span<const uint8>(cmdIDm, sizeof(cmdIDm)), rIDm.data(), nIDm) && IsOKSW(rIDm.data(), nIDm)) {
                    if (nIDm >= 10) { // body + SW1SW2
                        for (int i = 0; i < 8; ++i) {
                            got[i] = rIDm[i];
                        }
                        ok = true;
                    }
                }
            }

            SCardDisconnect(hCard, SCARD_LEAVE_CARD);

            if (ok) {
                {
                    std::scoped_lock lk(self->m_mutex);
                    self->m_idm = got;
                }
                self->m_ready.store(true, std::memory_order_release);
                self->m_ok.store(true, std::memory_order_release);
                break;
            }
        }

        // Step 3: Destory
        SCardReleaseContext(hCtx);
        self->m_running.store(false, std::memory_order_release);
    }
}
