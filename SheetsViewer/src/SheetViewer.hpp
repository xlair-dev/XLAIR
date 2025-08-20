#pragma once
#include <Siv3D.hpp>

class SheetViewer {
public:

    SheetViewer() = default;
    ~SheetViewer() = default;

    bool update();
    void draw() const;

private:

    struct UI {
        static constexpr int32 OffsetWidth = 300;

        static constexpr int32 LaneNum = 16;
        static constexpr int32 LaneWidth = 40;
        static constexpr int32 SideLaneWidth = 40;

        static constexpr int32 StageWidth = LaneWidth * LaneNum + 2 * SideLaneWidth;
    };;
};
