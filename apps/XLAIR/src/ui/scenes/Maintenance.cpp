#include "Maintenance.hpp"

#include "ui/assets/Assets.hpp"

#include <array>
#include <span>

namespace xlair::ui::scenes {
    namespace {
        constexpr std::array<StringView, 3> MenuItems{ U"スライダーテスト", U"サーバ通信", U"Title に戻る" };
        constexpr std::array<StringView, 4> ServerItems{ U"API ヘルスチェック",
                                                         U"同期する",
                                                         U"全件再検証・修復",
                                                         U"戻る" };

        void DrawMenu(const std::span<const StringView> items, const std::size_t selected) {
            const FontAsset font{ assets::font::Text };
            for (std::size_t index = 0; index < items.size(); ++index) {
                const double y = 128.0 + index * 52.0;
                if (index == selected) {
                    font(U"▶").draw(28, Vec2{ 48, y }, Palette::White);
                }
                font(items[index]).draw(28, Vec2{ 92, y }, Palette::White);
            }
        }

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

    Maintenance::Maintenance(const InitData& init) : SceneBase{ init } {
        auto& data = getData();
        if (data.music_select_context) {
            data.music_select_context->pause();
        }
        data.game_loader.cancel();
        data.maintenance_flow->reset();
        ClearPrint();
    }

    Maintenance::~Maintenance() {
        getData().maintenance_flow->reset();
    }

    bool Maintenance::busy() const {
        return getData().maintenance_flow->busy() || m_loading_jackets;
    }

    void Maintenance::update() {
        // The opening press is still down when a zero-duration transition updates the new scene.
        if (Scene::FrameCount() == m_opened_frame) {
            return;
        }
        updateServer();
        if (busy()) {
            return;
        }
        const auto* controller = getData().application->controller();
        if (controller && controller->maintenanceButton(app::controller::MaintenanceButton::Button1).down()) {
            changeScene(SceneState::Title, 0);
            return;
        }
        if (KeyLeft.down() && m_page != Page::Menu) {
            back();
            return;
        }
        if (m_page == Page::Slider) {
            updateSlider();
            return;
        }
        const std::size_t count = m_page == Page::Menu ? MenuItems.size() : ServerItems.size();
        if (KeyUp.down()) {
            m_selected_index = (m_selected_index + count - 1) % count;
        }
        if (KeyDown.down()) {
            m_selected_index = (m_selected_index + 1) % count;
        }
        if (!KeyEnter.down()) {
            return;
        }
        if (m_page == Page::Menu) {
            if (m_selected_index == 2) {
                changeScene(SceneState::Title, 0);
                return;
            }
            m_page = m_selected_index == 0 ? Page::Slider : Page::Server;
            m_selected_index = 0;
        } else if (m_selected_index == 0) {
            getData().maintenance_flow->checkHealth();
        } else if (m_selected_index == 1 || m_selected_index == 2) {
            // Discard the selection before replacing the catalog it references.
            getData().music_select_context.reset();
            getData().maintenance_flow->sync(
                m_selected_index == 2 ? app::flows::CatalogSync::Mode::VerifyAll
                                      : app::flows::CatalogSync::Mode::Incremental
            );
            m_sync_requested = true;
        } else {
            back();
        }
    }

    void Maintenance::back() {
        m_selected_index = m_page == Page::Server ? 1 : 0;
        m_page = Page::Menu;
    }

    void Maintenance::updateSlider() {
        auto* controller = getData().application->controller();
        if (!controller) {
            return;
        }
        if (const auto threshold = controller->touchThreshold()) {
            const int32 change = (KeyUp.down() ? 1 : 0) - (KeyDown.down() ? 1 : 0);
            if (change != 0) {
                controller->setTouchThreshold(
                    static_cast<uint8>(Clamp(static_cast<int32>(*threshold) + change, 1, 255))
                );
            }
        }
    }

    void Maintenance::updateServer() {
        auto& data = getData();
        auto& flow = *data.maintenance_flow;
        flow.update();
        if (m_sync_requested && flow.state() == app::flows::Maintenance::State::Succeeded) {
            data.jackets.start(data.application->musicCatalog());
            m_loading_jackets = true;
            m_sync_requested = false;
        } else if (flow.state() == app::flows::Maintenance::State::Failed) {
            m_sync_requested = false;
        }
        if (m_loading_jackets) {
            data.jackets.update();
            m_loading_jackets = data.jackets.state() != assets::JacketAssets::State::Ready;
        }
    }

    void Maintenance::draw() const {
        Scene::Rect().draw(Palette::Black);
        const FontAsset font{ assets::font::Text };
        const StringView title = m_page == Page::Slider   ? U"スライダーテスト"
                                 : m_page == Page::Server ? U"サーバ通信"
                                                          : U"MAINTENANCE";
        font(title).draw(36, Vec2{ 48, 40 }, Palette::White);
        if (m_page == Page::Slider) {
            drawSlider();
        } else if (m_page == Page::Server) {
            drawServer();
        } else {
            DrawMenu(MenuItems, m_selected_index);
        }
        const StringView guide = busy()                   ? U"処理中..."
                                 : m_page == Page::Slider ? U"↑ / ↓: 閾値を変更（起動中のみ）    ←: 戻る    F2: Title"
                                                          : U"↑ / ↓: 選択    Enter: 決定    ←: 戻る    F2: Title";
        font(guide).draw(20, Vec2{ 48, Scene::Height() - 56.0 }, Palette::White);
    }

    void Maintenance::drawSlider() const {
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

        constexpr double Left = 48;
        constexpr double Top = 210;
        const double width = (Scene::Width() - Left * 2) / 16.0;
        for (std::size_t index = 0; index < app::controller::TouchZoneCount; ++index) {
            const RectF cell{ Left + (index / 2) * width, Top + (index % 2) * 120.0, width, 120 };
            const auto value = controller->touchValue(index);
            const auto& button = controller->touchZone(index);
            const double red = value ? Min(1.0, *value / 254.0) : button.pressed() ? 1.0 : 0.0;
            cell.draw(ColorF{ red, 0, 0 }).drawFrame(1, Palette::White);
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

    void Maintenance::drawServer() const {
        DrawMenu(ServerItems, m_selected_index);
        const auto& flow = *getData().maintenance_flow;
        const FontAsset font{ assets::font::Text };
        if (const auto& config = getData().application->config()) {
            font(config->api.syncSource()).draw(22, RectF{ 48, 340, Scene::Width() - 96.0, 60 }, Palette::White);
        }
        const String message = m_loading_jackets ? U"Loading jackets..." : flow.message();
        font(message).draw(26, RectF{ 48, 420, Scene::Width() - 96.0, 400 }, Palette::White);
    }
}
