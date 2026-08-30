#include "Compiler.hpp"

namespace xlair::sheets::formats::sus {
    namespace detail {
        s3d::Optional<SideButton> SideButtonFromSideLongLane(const s3d::uint8 lane) {
            switch (lane) {
                case 0:
                case 1:
                    return SideButton::LeftUpper;
                case 2:
                case 3:
                    return SideButton::LeftLower;
                case 12:
                case 13:
                    return SideButton::RightLower;
                case 14:
                case 15:
                    return SideButton::RightUpper;
                default:
                    return s3d::none;
            }
        }
    }

    namespace {
        struct SideHoldBuilder {
            SideButton button = SideButton::LeftUpper;
            s3d::Array<SideHoldPoint> points;
        };

        [[nodiscard]] s3d::Optional<SideButton> ToSideButton(const DirectionalKind direction) {
            switch (direction) {
                case DirectionalKind::LeftUp:
                    return SideButton::LeftUpper;
                case DirectionalKind::RightUp:
                    return SideButton::RightUpper;
                case DirectionalKind::LeftDown:
                    return SideButton::LeftLower;
                case DirectionalKind::RightDown:
                    return SideButton::RightLower;
                case DirectionalKind::Up:
                case DirectionalKind::Down:
                    return s3d::none;
            }

            return s3d::none;
        }

        // SideLong (#mmm2xy) is compiled as a channel-based state machine:
        //
        //   Start (1?) -> resolve button once from the start lane
        //              -> create active_side_holds[channel]
        //   Relay      -> append to the active hold for channel
        //   End        -> append and finalize the active hold
        //
        // Start lanes 0-1, 2-3, C-D, and E-F select LeftUpper, LeftLower, RightLower, and RightUpper respectively.
        // Relay/End lanes do not re-select the button.
        //
        // This builder is kept private to the compiler because sus::Document should describe SUS source data, while
        // Chart should describe the physical controls of the game.
        using ActiveSideHolds = s3d::HashTable<ChannelId, SideHoldBuilder>;
    }

    Result<Chart> Compile(const Document& document, const CompileOptions& options) {
        (void)document;
        (void)options;
        (void)ToSideButton;
        (void)detail::SideButtonFromSideLongLane;
        (void)sizeof(ActiveSideHolds);

        return Result<Chart>::makeError(U"The new SUS compiler has not been migrated yet.");
    }
}
