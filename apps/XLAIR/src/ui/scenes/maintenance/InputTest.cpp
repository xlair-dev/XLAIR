#include "InputTest.hpp"

#include "ui/assets/Assets.hpp"

#include <array>

namespace xlair::ui::scenes::maintenance {
    namespace {
        void DrawButton(const RectF& rect, const StringView label, const app::controller::ButtonState& state) {
            rect.draw(state.pressed() ? Palette::Red : Palette::Black).drawFrame(1, Palette::White);
            FontAsset{ assets::font::Text }(label).drawAt(24, rect.center().movedBy(0, -16), Palette::White);
            FontAsset{
                assets::font::Text
            }(!state.available() ? U"--"
              : state.pressed()  ? U"ON"
                                 : U"OFF")
                .drawAt(24, rect.center().movedBy(0, 18), Palette::White);
        }
    }

    InputTest::InputTest(const InitData& init) : Base{ init } {
        if (const auto& config = getData().application->config()) {
            m_highlight_touch_threshold = config->controller.highlight_touch_threshold;
        }
    }

    void InputTest::update() {
        if (!navigate(SceneState::MaintenanceController)) {
            return;
        }
        if (KeyEnter.down()) {
            m_highlight_touch_threshold = !m_highlight_touch_threshold;
            getData().application->setTouchThresholdHighlight(m_highlight_touch_threshold);
        }
        auto* controller = getData().application->controller();
        if (!controller) {
            return;
        }
        if (const auto threshold = controller->touchThreshold()) {
            const int32 change = (KeyUp.down() ? 1 : 0) - (KeyDown.down() ? 1 : 0);
            if (change != 0) {
                getData().application->setTouchThreshold(
                    static_cast<uint8>(Clamp(static_cast<int32>(*threshold) + change, 1, 255))
                );
            }
        }
    }

    void InputTest::draw() const {
        drawFrame(U"入力テスト", U"↑ / ↓: 閾値    Enter: 緑表示切替    ←: 戻る    F2: Title");
        const auto& application = *getData().application;
        const auto* controller = getData().application->controller();
        const FontAsset font{ assets::font::Text };
        if (!controller) {
            font(U"Controller is not available.").draw(28, Vec2{ 48, 128 }, Palette::White);
            return;
        }
        const auto threshold = controller->touchThreshold();
        font(threshold ? U"閾値: {}"_fmt(static_cast<int32>(*threshold)) : U"閾値: -- (Keyboard)")
            .draw(28, Vec2{ 48, 128 }, Palette::White);
        font(U"しきい値超過を緑表示: {}"_fmt(m_highlight_touch_threshold ? U"ON" : U"OFF"))
            .draw(24, Vec2{ 48, 168 }, Palette::White);

        constexpr double Left = 48;
        constexpr double Top = 210;
        const double width = (Scene::Width() - Left * 2) / 16.0;
        for (std::size_t index = 0; index < app::controller::TouchZoneCount; ++index) {
            const RectF cell{ Left + (index / 2) * width, Top + (index % 2) * 120.0, width, 120 };
            const auto value = controller->touchValue(index);
            const auto& button = controller->touchZone(index);
            const double red = value ? Min(1.0, *value / 254.0) : button.pressed() ? 1.0 : 0.0;
            const bool above_threshold = m_highlight_touch_threshold && threshold && value && *value >= *threshold;
            cell.draw(above_threshold ? ColorF{ 0, 1, 0 } : ColorF{ red, 0, 0 }).drawFrame(1, Palette::White);
            font(index).drawAt(18, cell.center().movedBy(0, -30), Palette::White);
            font(value ? Format(static_cast<int32>(*value)) : U"--")
                .drawAt(30, cell.center().movedBy(0, 8), Palette::White);
        }

        constexpr std::array<StringView, 4> SideLabels{ U"左上", U"左下", U"右上", U"右下" };
        for (std::size_t index = 0; index < SideLabels.size(); ++index) {
            DrawButton(
                RectF{ Left + index * 180.0, 500, 160, 86 },
                SideLabels[index],
                controller->sideButton(static_cast<app::controller::SideButton>(index))
            );
            DrawButton(
                RectF{ Left + index * 180.0, 620, 160, 86 },
                U"メンテ {}"_fmt(index + 1),
                controller->maintenanceButton(static_cast<app::controller::MaintenanceButton>(index))
            );
        }
        if (const auto& error = application.controllerError()) {
            font(error->message).draw(24, RectF{ 48, 760, Scene::Width() - 96.0, 200 }, Palette::White);
        }
    }
}
