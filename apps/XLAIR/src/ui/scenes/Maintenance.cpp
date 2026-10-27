#include "Maintenance.hpp"

#include "ui/assets/Assets.hpp"

#include <array>

namespace xlair::ui::scenes {
    namespace {
        constexpr std::array<StringView, 7> MenuItems{
            U"コントローラテスト", U"カードリーダーテスト", U"サーバ通信",   U"サウンド",
            U"オートプレイ",       U"設定を保存",           U"Title に戻る",
        };
    }

    Maintenance::Maintenance(const InitData& init) : maintenance::Base{ init } {
        auto& data = getData();
        if (data.music_select_context) {
            data.music_select_context->pause();
        }
        data.game_loader.cancel();
        ClearPrint();
    }

    void Maintenance::update() {
        if (!navigate()) {
            return;
        }
        auto& data = getData();
        auto& selected = data.maintenance_menu_index;
        maintenance::UpdateMenuSelection(selected, MenuItems);
        if (!KeyEnter.down()) {
            return;
        }
        switch (selected) {
            case 0:
                data.maintenance_controller_index = 0;
                changeScene(SceneState::MaintenanceController, 0);
                break;
            case 1:
                changeScene(SceneState::MaintenanceCardReaderTest, 0);
                break;
            case 2:
                changeScene(SceneState::MaintenanceServer, 0);
                break;
            case 3:
                changeScene(SceneState::MaintenanceSound, 0);
                break;
            case 4:
                data.application->setAutoplay(!data.application->autoplay());
                break;
            case 5:
                data.maintenance_flow->saveConfig();
                break;
            default:
                returnToTitle();
                break;
        }
    }

    void Maintenance::draw() const {
        drawFrame(U"MAINTENANCE");
        maintenance::DrawMenu(MenuItems, getData().maintenance_menu_index);
        const auto& application = *getData().application;
        const auto& flow = *getData().maintenance_flow;
        const FontAsset font{ assets::font::Text };
        const bool autoplay = application.autoplay();
        const bool unsaved = application.hasUnsavedConfig();
        font(autoplay ? U"ON" : U"OFF")
            .draw(
                28,
                Vec2{ 480, maintenance::MenuTop + 4 * maintenance::MenuRowHeight },
                autoplay ? Palette::Green : Palette::Gray
            );
        font(unsaved ? U"未保存の変更があります" : U"未保存の変更はありません")
            .draw(24, Vec2{ 48, 540 }, unsaved ? Palette::Yellow : Palette::Gray);
        if (!unsaved || flow.configSaveFailed()) {
            font(flow.configMessage())
                .draw(
                    24,
                    RectF{ 48, 590, Scene::Width() - 96.0, 250 },
                    flow.configSaveFailed() ? Palette::Red : Palette::White
                );
        }
    }
}
