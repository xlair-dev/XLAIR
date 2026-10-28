#pragma once

#include "ui/Scene.hpp"

#include <span>

namespace xlair::ui::scenes::maintenance {
    inline constexpr double MenuTop = 128.0;
    inline constexpr double MenuRowHeight = 52.0;
    inline constexpr StringView MenuGuide = U"↑ / ↓: 選択    Enter: 決定    ←: 戻る    F2: Title";

    class Base : public ui::SceneBase {
    public:
        explicit Base(const InitData& init);

    protected:
        bool navigate(Optional<SceneState> parent = none);
        void returnToTitle();
        void drawFrame(StringView title, StringView guide = MenuGuide) const;

    private:
        int32 m_opened_frame = Scene::FrameCount();
    };

    bool UpdateMenuSelection(
        std::size_t& selected,
        std::span<const StringView> items,
        Optional<std::size_t> disabled = none
    );

    void DrawMenu(std::span<const StringView> items, std::size_t selected, Optional<std::size_t> disabled = none);

    class ValueRepeat {
    public:
        void reset();
        int32 delta(int32 max_steps);

    private:
        int32 m_direction = 0;
        double m_hold_seconds = 0.0;
        double m_next_repeat_seconds = 0.4;
    };
}
