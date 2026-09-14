#include "Boot.hpp"

namespace xlair::ui::scenes {
    namespace {
        void ApplyConfig(const app::Config& config) {
            Window::SetStyle(config.window.sizable ? WindowStyle::Sizable : WindowStyle::Fixed);
            Window::Resize(config.window.width, config.window.height);
            Window::SetFullscreen(config.window.fullscreen);
            Scene::SetLetterbox(config.window.letterbox_color);
        }
    }

    Boot::Boot(const InitData& init) : SceneBase{ init } {
        Print << U"[Boot] Loading config...";
    }

    void Boot::update() {
        if (m_state == State::Waiting) {
            m_state = State::Loading;
            return;
        }

        if (m_state == State::Loading) {
            if (!getData().loadConfig()) {
                const auto& error = getData().configLoadError();
                Print << U"[Boot] "
                      << (error ? U"Failed to load config.\n{}\n{}"_fmt(error->message, error->path)
                                : U"Failed to load config.");
                m_state = State::Failed;
                return;
            }

            ApplyConfig(*getData().config());
            Print << U"[Boot] Config loaded.";
            m_state = State::Loaded;
            return;
        }

        if (m_state == State::Loaded) {
            Print << U"[Boot] Initializing API client...";
            if (!getData().initializeApi()) {
                Print << U"[Boot] Failed to initialize the API client.";
                m_state = State::Failed;
                return;
            }
            Print << U"[Boot] Sync starts in 10 seconds. Press N to skip.";
            m_sync_wait.restart();
            m_state = State::WaitingForSync;
            return;
        }

        if (m_state == State::WaitingForSync) {
            // Prefer explicit input even on the frame the countdown expires.
            if (KeyN.down()) {
                m_sync_wait.pause();
                Print << U"[Boot] Catalog sync skipped.";
                Logger << U"[Boot] Catalog sync skipped.";
                m_state = State::Ready;
                return;
            }
            if (m_sync_wait.sF() < 10.0) {
                return;
            }
            m_sync_wait.pause();
            Print << U"[Boot] Authenticating device and syncing catalog...";
            Logger << U"[Boot] Starting catalog sync.";
            getData().startCatalogSync();
            m_state = State::Syncing;
            return;
        }

        if (m_state == State::Syncing) {
            getData().updateCatalogSync();
            const auto state = getData().catalogSyncState();
            if (state == app::Application::CatalogSyncState::Succeeded) {
                const auto& catalog = getData().catalog();
                std::size_t sheet_count = 0;
                for (const auto& music : catalog) {
                    sheet_count += music.sheets.size();
                }
                const String message =
                    U"[Boot] Catalog synced: {} music entries, {} sheets."_fmt(catalog.size(), sheet_count);
                Print << message;
                Logger << message;
                m_state = State::Ready;
            } else if (state == app::Application::CatalogSyncState::Failed) {
                const auto& error = getData().catalogSyncError();
                String message = U"[Boot] Catalog sync failed.";
                if (error) {
                    message += U"\n" + error->message;
                    if (error->status_code) {
                        message += U"\nHTTP {}"_fmt(*error->status_code);
                    }
                }
                Print << message << U"\nPress R to retry.";
                Logger << message;
                m_state = State::SyncFailed;
            }
            return;
        }

        if (m_state == State::SyncFailed && KeyR.down()) {
            Print << U"[Boot] Retrying catalog sync...";
            getData().startCatalogSync();
            m_state = State::Syncing;
            return;
        }

        if (m_state == State::Ready) {
            // ClearPrint();
            changeScene(SceneState::Title, 0);
        }
    }

    void Boot::draw() const {
        Scene::Rect().draw(Palette::Black);
    }
}
