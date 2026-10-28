#include "Server.hpp"

#include "ui/assets/Assets.hpp"

#include <array>

namespace xlair::ui::scenes::maintenance {
    namespace {
        constexpr std::array<StringView, 5> ServerItems{
            U"API タイムアウト", U"API ヘルスチェック", U"同期する", U"全件再検証・修復", U"戻る",
        };
    }

    Server::Server(const InitData& init) : Base{ init } {}

    Server::~Server() {
        // Normal navigation is blocked while busy; shutdown must still cancel pending work.
        if (getData().maintenance_flow->busy()) {
            getData().maintenance_flow->reset();
        }
    }

    bool Server::busy() const {
        return getData().maintenance_flow->busy() || m_loading_jackets;
    }

    void Server::update() {
        updateOperation();
        if (busy()) {
            return;
        }
        const Optional<SceneState> parent =
            m_selected_index == 0 ? none : Optional<SceneState>{ SceneState::Maintenance };
        if (!navigate(parent)) {
            return;
        }
        if (UpdateMenuSelection(m_selected_index, ServerItems)) {
            m_value_repeat.reset();
        }
        if (m_selected_index == 0) {
            const int32 delta = m_value_repeat.delta(299);
            if (delta != 0) {
                if (const auto& config = getData().application->config()) {
                    const double seconds = Clamp(config->api.timeout_seconds + delta, 1.0, 300.0);
                    getData().application->setApiTimeoutSeconds(seconds);
                }
            }
            return;
        }
        if (!KeyEnter.down()) {
            return;
        }
        auto& data = getData();
        if (m_selected_index == 1) {
            data.maintenance_flow->checkHealth();
        } else if (m_selected_index == 2 || m_selected_index == 3) {
            // Discard the selection before replacing the catalog it references.
            data.music_select_context.reset();
            data.maintenance_flow->sync(
                m_selected_index == 3 ? app::flows::CatalogSync::Mode::VerifyAll
                                      : app::flows::CatalogSync::Mode::Incremental
            );
            m_sync_requested = true;
        } else {
            changeScene(SceneState::Maintenance, 0);
        }
    }

    void Server::updateOperation() {
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

    void Server::draw() const {
        const StringView guide = busy() ? U"処理中..."
                                 : m_selected_index == 0
                                     ? U"↑ / ↓: 選択    ← / →: タイムアウト（長押し可）    F2: Title"
                                     : MenuGuide;
        drawFrame(U"サーバ通信", guide);
        DrawMenu(ServerItems, m_selected_index);
        const double endpoint_y = MenuTop + ServerItems.size() * MenuRowHeight + 24.0;
        const double status_y = endpoint_y + 84.0;
        const double progress_y = status_y + 128.0;
        const auto& flow = *getData().maintenance_flow;
        const FontAsset font{ assets::font::Text };
        if (const auto& config = getData().application->config()) {
            font(U"{} 秒"_fmt(config->api.timeout_seconds)).draw(28, Vec2{ 580, 128 }, Palette::White);
        }
        if (const auto& config = getData().application->config()) {
            font(config->api.syncSource()).draw(22, RectF{ 48, endpoint_y, Scene::Width() - 96.0, 60 }, Palette::White);
        }
        const String message = m_loading_jackets ? U"Loading jackets..." : flow.message();
        font(message).draw(26, RectF{ 48, status_y, Scene::Width() - 96.0, 104 }, Palette::White);
        if (const auto progress = flow.progress(); progress && progress->total_files > 0) {
            const RectF bar{ 48, progress_y, Scene::Width() - 96.0, 24 };
            bar.draw(Palette::Gray);
            const double active_fraction =
                progress->download_total_bytes && *progress->download_total_bytes > 0
                    ? Clamp(static_cast<double>(progress->downloaded_bytes) / *progress->download_total_bytes, 0.0, 1.0)
                    : 0.0;
            const double fraction =
                Clamp((progress->completed_files + active_fraction) / progress->total_files, 0.0, 1.0);
            RectF{ bar.x, bar.y, bar.w * fraction, bar.h }.draw(Palette::White);
            font(U"完了ファイル: {}/{}"_fmt(progress->completed_files, progress->total_files))
                .draw(24, Vec2{ 48, progress_y + 44 }, Palette::White);
        }
    }
}
