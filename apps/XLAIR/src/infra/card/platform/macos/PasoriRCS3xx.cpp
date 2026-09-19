#include "infra/card/PasoriRCS3xx.hpp"

#include <PCSC/winscard.h>

#include <cstring>
#include <string>

#include "infra/card/platform/common/PcscScan.hpp"

namespace xlair::infra::card {
    namespace {
        struct MacOsPcscApi {
            using Context = SCARDCONTEXT;
            using Card = SCARDHANDLE;
            using NativeChar = char;
            using NativeString = std::string;
            using ReaderState = SCARD_READERSTATE;
            using IoRequest = SCARD_IO_REQUEST;
            using Size = uint32_t;
            using Protocol = uint32_t;
            using Status = int32_t;

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
                return std::strlen(value);
            }

            [[nodiscard]]
            static String Decode(const NativeChar* value) {
                return Unicode::FromUTF8(value);
            }

            [[nodiscard]]
            static NativeString Encode(const String& value) {
                return value.toUTF8();
            }

            [[nodiscard]]
            static Size UnawareState() {
                return SCARD_STATE_UNAWARE;
            }

            [[nodiscard]]
            static bool IsCardPresent(const ReaderState& state) {
                return (state.dwEventState & SCARD_STATE_PRESENT) != 0;
            }

            static int32_t ListReaders(const SCARDCONTEXT context, NativeChar* readers, Size* size) {
                return SCardListReaders(context, nullptr, readers, size);
            }

            static int32_t GetStatusChange(const SCARDCONTEXT context, ReaderState* state) {
                return SCardGetStatusChange(context, 200, state, 1);
            }

            static int32_t
            Connect(const SCARDCONTEXT context, const NativeChar* reader, SCARDHANDLE* card, Size* protocol) {
                return SCardConnect(
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
        return detail::MakePcscScan<MacOsPcscApi>();
    }
}
