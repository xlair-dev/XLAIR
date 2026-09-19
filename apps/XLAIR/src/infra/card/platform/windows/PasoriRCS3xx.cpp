#include "infra/card/PasoriRCS3xx.hpp"

#include <cwchar>
#include <string>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <winscard.h>

#include "infra/card/platform/common/PcscScan.hpp"

namespace xlair::infra::card {
    namespace {
        struct WindowsPcscApi {
            using Context = SCARDCONTEXT;
            using Card = SCARDHANDLE;
            using NativeChar = wchar_t;
            using NativeString = std::wstring;
            using ReaderState = SCARD_READERSTATEW;
            using IoRequest = SCARD_IO_REQUEST;
            using Size = DWORD;
            using Protocol = DWORD;
            using Status = LONG;

            [[nodiscard]]
            static bool Succeeded(const Status status) {
                return status == SCARD_S_SUCCESS;
            }

            [[nodiscard]]
            static bool TimedOut(const Status status) {
                return status == SCARD_E_TIMEOUT;
            }

            static Status EstablishContext(Context* context) {
                return SCardEstablishContext(SCARD_SCOPE_USER, nullptr, nullptr, context);
            }

            static void ReleaseContext(const Context context) {
                SCardReleaseContext(context);
            }

            static void Disconnect(const Card card) {
                SCardDisconnect(card, SCARD_LEAVE_CARD);
            }

            [[nodiscard]]
            static std::size_t Length(const NativeChar* value) {
                return std::wcslen(value);
            }

            [[nodiscard]]
            static String Decode(const NativeChar* value) {
                return Unicode::FromWstring(value);
            }

            [[nodiscard]]
            static NativeString Encode(const String& value) {
                return value.toWstr();
            }

            [[nodiscard]]
            static Size UnawareState() {
                return SCARD_STATE_UNAWARE;
            }

            [[nodiscard]]
            static bool IsCardPresent(const ReaderState& state) {
                return (state.dwEventState & SCARD_STATE_PRESENT) != 0;
            }

            static LONG ListReaders(const SCARDCONTEXT context, NativeChar* readers, Size* size) {
                return SCardListReadersW(context, nullptr, readers, size);
            }

            static LONG GetStatusChange(const SCARDCONTEXT context, ReaderState* state) {
                return SCardGetStatusChangeW(context, 200, state, 1);
            }

            static LONG
            Connect(const SCARDCONTEXT context, const NativeChar* reader, SCARDHANDLE* card, Size* protocol) {
                return SCardConnectW(
                    context,
                    reader,
                    SCARD_SHARE_SHARED,
                    SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1,
                    card,
                    protocol
                );
            }

            [[nodiscard]]
            static const IoRequest* ProtocolInfo(const Protocol protocol) {
                return protocol == SCARD_PROTOCOL_T0 ? SCARD_PCI_T0 : SCARD_PCI_T1;
            }

            static Status Transmit(
                const Card card,
                const IoRequest* protocol,
                const uint8* command,
                const Size command_size,
                uint8* response,
                Size* response_size
            ) {
                return SCardTransmit(card, protocol, command, command_size, nullptr, response, response_size);
            }
        };
    }

    app::card::ScanRequest PasoriRCS3xx::scan() {
        return detail::MakePcscScan<WindowsPcscApi>();
    }
}
