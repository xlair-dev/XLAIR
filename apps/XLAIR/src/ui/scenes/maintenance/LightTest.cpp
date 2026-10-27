#include "LightTest.hpp"

#include "ui/assets/Assets.hpp"

#include <array>

namespace xlair::ui::scenes::maintenance {
    namespace {
        constexpr std::array<StringView, 7> LightItems{
            U"消灯", U"赤", U"緑", U"青", U"白", U"順番に点灯", U"入力に連動して点灯",
        };
        constexpr std::array LightPatterns{
            app::flows::LightTest::Pattern::Off,   app::flows::LightTest::Pattern::Red,
            app::flows::LightTest::Pattern::Green, app::flows::LightTest::Pattern::Blue,
            app::flows::LightTest::Pattern::White, app::flows::LightTest::Pattern::Sequence,
            app::flows::LightTest::Pattern::Input,
        };
    }

    LightTest::LightTest(const InitData& init) : Base{ init } {
        if (auto* controller = getData().application->controller()) {
            m_light_test = std::make_unique<app::flows::LightTest>(*controller);
        }
    }

    void LightTest::update() {
        if (!navigate(SceneState::MaintenanceController) || !m_light_test || !m_light_test->supported()) {
            return;
        }
        if (KeyRight.down()) {
            m_light_test->changeTarget();
        }
        m_light_test->update(Scene::DeltaTime());
        UpdateMenuSelection(m_selected_index, LightItems);
        if (KeyEnter.down()) {
            m_light_test->start(LightPatterns[m_selected_index]);
        }
    }

    void LightTest::draw() const {
        drawFrame(U"LED テスト", U"↑ / ↓: 選択    Enter: 点灯    →: 出力先切替    ←: 戻る    F2: Title");
        DrawMenu(LightItems, m_selected_index);
        const FontAsset font{ assets::font::Text };
        if (!m_light_test || !m_light_test->supported()) {
            font(U"このコントローラは LED 出力に対応していません。").draw(26, Vec2{ 530, 128 }, Palette::White);
            return;
        }
        const auto target = m_light_test->target();
        font(target == app::flows::LightTest::Target::Slider ? U"出力: スライダー" : U"出力: サイド")
            .draw(28, Vec2{ 530, 128 }, Palette::White);
        if (const auto active = m_light_test->activePattern()) {
            font(U"実行中: {}"_fmt(LightItems[static_cast<std::size_t>(*active)]))
                .draw(26, Vec2{ 530, 190 }, Palette::White);
        }
        if (const auto& error = m_light_test->error()) {
            font(error->message).draw(26, RectF{ 530, 270, Scene::Width() - 578.0, 300 }, Palette::White);
        }
    }
}
