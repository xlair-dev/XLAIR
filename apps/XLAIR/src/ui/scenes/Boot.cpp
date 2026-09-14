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
        auto& flow = *getData().boot_flow;
        const auto previous = flow.state();

        if (previous == app::BootFlow::State::WaitingForSync && KeyN.down()) {
            flow.skipSync();
        } else if (previous == app::BootFlow::State::SyncFailed && KeyR.down()) {
            flow.retrySync();
        } else {
            flow.update(Scene::DeltaTime());
        }

        if (flow.state() != previous) {
            handleStateChange(previous);
        }
        if (flow.state() == app::BootFlow::State::Ready) {
            changeScene(SceneState::Title, 0);
        }
    }

    void Boot::draw() const {
        Scene::Rect().draw(Palette::Black);
    }

    void Boot::handleStateChange(const app::BootFlow::State previous) {
        const auto& data = getData();
        switch (data.boot_flow->state()) {
            case app::BootFlow::State::LoadingConfig:
                break;

            case app::BootFlow::State::WaitingForSync:
                ApplyConfig(*data.application->config());
                Print << U"[Boot] Config loaded.";
                Print << U"[Boot] API client initialized.";
                Print << U"[Boot] Sync starts in 10 seconds. Press N to skip.";
                break;

            case app::BootFlow::State::Syncing: {
                const String message = previous == app::BootFlow::State::SyncFailed
                                           ? U"[Boot] Retrying catalog sync..."
                                           : U"[Boot] Authenticating device and syncing catalog...";
                Print << message;
                Logger << message;
                break;
            }

            case app::BootFlow::State::SyncFailed: {
                String message = U"[Boot] Catalog sync failed.";
                if (const auto& error = data.boot_flow->syncError()) {
                    message += U"\n" + error->message;
                    if (error->status_code) {
                        message += U"\nHTTP {}"_fmt(*error->status_code);
                    }
                }
                Print << message << U"\nPress R to retry.";
                Logger << message;
                break;
            }

            case app::BootFlow::State::Ready:
                if (previous == app::BootFlow::State::WaitingForSync) {
                    Print << U"[Boot] Catalog sync skipped.";
                    Logger << U"[Boot] Catalog sync skipped.";
                } else {
                    const auto& catalog = data.application->catalog();
                    std::size_t sheet_count = 0;
                    for (const auto& music : catalog) {
                        sheet_count += music.sheets.size();
                    }
                    const String message =
                        U"[Boot] Catalog synced: {} music entries, {} sheets."_fmt(catalog.size(), sheet_count);
                    Print << message;
                    Logger << message;
                }
                break;

            case app::BootFlow::State::Failed: {
                const auto& error = data.boot_flow->configError();
                Print << U"[Boot] "
                      << (error ? U"Failed to initialize.\n{}\n{}"_fmt(error->message, error->path)
                                : U"Failed to initialize.");
                break;
            }
        }
    }
}
