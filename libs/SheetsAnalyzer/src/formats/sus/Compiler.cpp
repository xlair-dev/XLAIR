#include "Compiler.hpp"

namespace xlair::sheets::formats::sus {
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
        // The exact lane -> SideButton table is intentionally not guessed here until the chart convention is fixed.
        // Importantly, Relay/End lanes do not re-select the button.
        //
        // This builder is kept private to the compiler because sus::Document should describe SUS source data, while
        // Chart should describe the physical controls of the game.
        using ActiveSideHolds = s3d::HashTable<ChannelId, SideHoldBuilder>;
    }

    Result<Chart> Compile(const Document& document, const CompileOptions& options) {
        (void)document;
        (void)options;
        (void)ToSideButton;
        (void)sizeof(ActiveSideHolds);

        Result<Chart> result;
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::Error,
            .message = U"The new SUS compiler has not been migrated yet.",
            .path = U"",
            .line = s3d::none,
            .column = s3d::none,
        });
        return result;
    }
}
