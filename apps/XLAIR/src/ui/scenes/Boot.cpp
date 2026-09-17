#include "Boot.hpp"

namespace xlair::ui::scenes {
    namespace {
        void ApplyConfig(const app::Config& config) {
            Window::SetStyle(config.window.sizable ? WindowStyle::Sizable : WindowStyle::Fixed);
            Window::Resize(config.window.width, config.window.height);
            Window::SetFullscreen(config.window.fullscreen);
            Scene::SetLetterbox(config.window.letterbox_color);
        }

        void ReportBoot(const StringView message) {
            Print << message;
            Logger << message;
        }

        String FormatDiagnostic(const sheets::Diagnostic& diagnostic) {
            String location = diagnostic.path;
            if (diagnostic.line) {
                location += U":{}"_fmt(*diagnostic.line);
                if (diagnostic.column) {
                    location += U":{}"_fmt(*diagnostic.column);
                }
            }
            return location.isEmpty() ? diagnostic.message : location + U": " + diagnostic.message;
        }
    }

    Boot::Boot(const InitData& init) : SceneBase{ init } {
        ReportBoot(U"[Boot] Loading config...");
    }

    void Boot::update() {
        auto& flow = *getData().boot_flow;
        const auto previous = flow.state();

        if (previous == app::BootFlow::State::WaitingForSync && KeyN.down()) {
            flow.skipSync();
        } else if (previous == app::BootFlow::State::SyncFailed && KeyR.down()) {
            flow.retrySync();
        } else if (previous == app::BootFlow::State::MetadataFailed && KeyR.down()) {
            flow.retryMetadata();
        } else {
            flow.update(Scene::DeltaTime());
        }

        if (flow.state() != previous) {
            handleStateChange(previous);
        }
        if (flow.state() == app::BootFlow::State::Ready) {
            updateJacketLoading();
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
                ReportBoot(U"[Boot] Config loaded.");
                ReportBoot(U"[Boot] API client initialized.");
                ReportBoot(U"[Boot] Sync starts in 10 seconds. Press N to skip.");
                break;

            case app::BootFlow::State::Syncing: {
                const String message = previous == app::BootFlow::State::SyncFailed
                                           ? U"[Boot] Retrying catalog sync..."
                                           : U"[Boot] Authenticating device and syncing catalog...";
                ReportBoot(message);
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
                ReportBoot(message + U"\nPress R to retry.");
                break;
            }

            case app::BootFlow::State::LoadingMetadata:
                if (previous == app::BootFlow::State::WaitingForSync) {
                    ReportBoot(U"[Boot] Catalog sync skipped.");
                } else if (previous == app::BootFlow::State::MetadataFailed) {
                    ReportBoot(U"[Boot] Retrying sheet metadata load...");
                } else {
                    ReportBoot(U"[Boot] Catalog synced.");
                }
                ReportBoot(U"[Boot] Loading sheet metadata...");
                break;

            case app::BootFlow::State::MetadataFailed: {
                const auto& diagnostics = data.boot_flow->metadataDiagnostics();
                String message = U"[Boot] Failed to load sheet metadata.";
                if (!diagnostics.isEmpty()) {
                    message += U"\n" + FormatDiagnostic(diagnostics.front());
                }
                ReportBoot(message + U"\nPress R to retry.");
                for (const auto& diagnostic : diagnostics) {
                    Logger << U"[SheetsAnalyzer] " + FormatDiagnostic(diagnostic);
                }
                break;
            }

            case app::BootFlow::State::Ready: {
                const auto& catalog = data.application->musicCatalog();
                std::size_t difficulty_count = 0;
                for (const auto& music : catalog) {
                    difficulty_count += music.difficulties.size();
                }
                const String message =
                    U"[Boot] Metadata loaded: {} music entries, {} difficulties."_fmt(catalog.size(), difficulty_count);
                ReportBoot(message);
                break;
            }

            case app::BootFlow::State::Failed: {
                const auto& error = data.boot_flow->configError();
                ReportBoot(
                    U"[Boot] " + (error ? U"Failed to initialize.\n{}\n{}"_fmt(error->message, error->path)
                                        : U"Failed to initialize.")
                );
                break;
            }
        }
    }

    void Boot::updateJacketLoading() {
        auto& data = getData();
        auto& jackets = data.jackets;
        if (!m_jacket_loading_started) {
            jackets.start(data.application->musicCatalog());
            m_jacket_loading_started = true;
            ReportBoot(U"[Boot] Loading jackets...");
        }

        jackets.update();
        if (jackets.state() != assets::JacketAssets::State::Ready) {
            return;
        }

        for (const auto& diagnostic : jackets.diagnostics()) {
            Logger << U"[Jacket] " + FormatDiagnostic(diagnostic);
        }
        const String message = U"[Boot] Jackets loaded: {}/{} ({} fallback)."_fmt(
            jackets.completedCount(),
            jackets.totalCount(),
            jackets.fallbackCount()
        );
        ReportBoot(message);
        ClearPrint();
        changeScene(SceneState::Title, 0);
    }
}
