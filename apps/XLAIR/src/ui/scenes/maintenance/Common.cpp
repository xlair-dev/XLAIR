#include "Common.hpp"

#include "ui/assets/Assets.hpp"

namespace xlair::ui::scenes::maintenance {
    namespace {
        constexpr double ValueRepeatDelay = 0.4;
        constexpr double ValueRepeatInterval = 0.1;
        constexpr double ValueFastRepeatAfter = 1.5;
        constexpr double ValueFastRepeatInterval = 0.03;
    }

    Base::Base(const InitData& init) : ui::SceneBase{ init } {}

    bool Base::navigate(const Optional<SceneState> parent) {
        // A zero-duration transition may update the new scene with the same key press.
        if (Scene::FrameCount() == m_opened_frame) {
            return false;
        }
        const auto* controller = getData().application->controller();
        if (controller && controller->maintenanceButton(app::controller::MaintenanceButton::Button1).down()) {
            returnToTitle();
            return false;
        }
        if (parent && KeyLeft.down()) {
            changeScene(*parent, 0);
            return false;
        }
        return true;
    }

    void Base::returnToTitle() {
        getData().maintenance_flow->reset();
        changeScene(SceneState::Title, 0);
    }

    void Base::drawFrame(const StringView title, const StringView guide) const {
        Scene::Rect().draw(Palette::Black);
        const FontAsset font{ assets::font::Text };
        font(title).draw(36, Vec2{ 48, 40 }, Palette::White);
        font(guide).draw(20, Vec2{ 48, Scene::Height() - 56.0 }, Palette::White);
    }

    bool UpdateMenuSelection(
        std::size_t& selected,
        const std::span<const StringView> items,
        const Optional<std::size_t> disabled
    ) {
        const int32 direction = (KeyDown.down() ? 1 : 0) - (KeyUp.down() ? 1 : 0);
        if (direction == 0 || items.empty()) {
            return false;
        }
        do {
            selected = (selected + items.size() + direction) % items.size();
        } while (disabled && selected == *disabled);
        return true;
    }

    void DrawMenu(
        const std::span<const StringView> items,
        const std::size_t selected,
        const Optional<std::size_t> disabled
    ) {
        const FontAsset font{ assets::font::Text };
        for (std::size_t index = 0; index < items.size(); ++index) {
            const double y = MenuTop + index * MenuRowHeight;
            if (index == selected) {
                font(U"▶").draw(28, Vec2{ 48, y }, Palette::White);
            }
            const bool enabled = !disabled || *disabled != index;
            font(items[index]).draw(28, Vec2{ 92, y }, enabled ? Palette::White : Palette::Gray);
            if (!enabled) {
                font(U"（未対応）").draw(24, Vec2{ 290, y + 3 }, Palette::Gray);
            }
        }
    }

    void ValueRepeat::reset() {
        m_direction = 0;
        m_hold_seconds = 0.0;
        m_next_repeat_seconds = ValueRepeatDelay;
    }

    int32 ValueRepeat::delta(const int32 max_steps) {
        const int32 direction = (KeyRight.pressed() ? 1 : 0) - (KeyLeft.pressed() ? 1 : 0);
        if (direction == 0) {
            reset();
            return 0;
        }

        int32 steps = 0;
        if (direction != m_direction) {
            reset();
            m_direction = direction;
            steps = 1;
        } else {
            m_hold_seconds += Scene::DeltaTime();
            // Catch up across slow frames, bounded by the setting's full range.
            while (m_hold_seconds >= m_next_repeat_seconds && steps < max_steps) {
                ++steps;
                m_next_repeat_seconds +=
                    m_next_repeat_seconds >= ValueFastRepeatAfter ? ValueFastRepeatInterval : ValueRepeatInterval;
            }
            if (steps == max_steps) {
                m_next_repeat_seconds = m_hold_seconds + ValueFastRepeatInterval;
            }
        }
        return direction * steps;
    }
}
