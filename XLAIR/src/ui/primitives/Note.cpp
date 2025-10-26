#include "Note.hpp"

namespace ui::primitives {
    void DrawTapNote(const Vec2& left_center, double width, double height) {
        const Transformer2D t{ Mat3x2::Rotate(-90_deg, left_center) }; // NOTE: Siv3D v0.6 limitation: cannot draw horizontal gradient RoundRect directly
        RoundRect{ left_center.movedBy(-height / 2, 0), height, width, 4.0 }
            .drawShadow(Vec2{ -10, 0 }, 30.0, 0, ColorF{ U"#BEE4EF" }.withA(0.5))
            .draw(Arg::top = ColorF{ U"#FFFFFF" }, Arg::bottom = ColorF{ U"#BEE4EF" })
            .movedBy(10, 0)
            .stretched(0, -5)
            .drawFrame(2.0, Arg::top = ColorF{ U"#BEE4EF" }, Arg::bottom = ColorF{ U"#FFFFFF" });
    }

    void DrawXTapNote(const Vec2& left_center, double width, double height) {
        const Transformer2D t{ Mat3x2::Rotate(-90_deg, left_center) }; // NOTE: Siv3D v0.6 limitation: cannot draw horizontal gradient RoundRect directly
        RoundRect{ left_center.movedBy(-height / 2, 0), height, width, 4.0 }
            .drawShadow(Vec2{ -10, 0 }, 30.0, 0, ColorF{ U"#FEDF64" }.withA(0.5))
            .draw(Arg::top = ColorF{ U"#FEEFB4" }, Arg::bottom = ColorF{ U"#FEDF64" })
            .movedBy(10, 0)
            .stretched(0, -5)
            .drawFrame(2.0, Arg::top = ColorF{ U"#FEDF64" }, Arg::bottom = ColorF{ U"#FEEFB4" });
    }

    void DrawFlickNote(const Vec2& left_center, double width, double height) {
        // TODO: implement
        const Transformer2D t{ Mat3x2::Rotate(-90_deg, left_center) }; // NOTE: Siv3D v0.6 limitation: cannot draw horizontal gradient RoundRect directly
        RoundRect{ left_center.movedBy(-height / 2, 0), height, width, 4.0 }
            .drawShadow(Vec2{ -10, 0 }, 30.0, 0, ColorF{ U"##B429FA" }.withA(0.5))
            .draw(Arg::top = ColorF{ U"#D5A1F0" }, Arg::bottom = ColorF{ U"#B429FA" });
    }

    void DrawHoldNoteHead(const Vec2& left_center, double width, double height) {
        const Transformer2D t{ Mat3x2::Rotate(-90_deg, left_center) }; // NOTE: Siv3D v0.6 limitation: cannot draw horizontal gradient RoundRect directly
        RoundRect{ left_center.movedBy(-height / 2, 0), height, width, 4.0 }
            .drawShadow(Vec2{ -10, 0 }, 30.0, 0, ColorF{ U"#43A4F2" }.withA(0.5))
            .draw(Arg::top = ColorF{ U"#55C8FF" }, Arg::bottom = ColorF{ U"#43A4F2" });
    }
}
