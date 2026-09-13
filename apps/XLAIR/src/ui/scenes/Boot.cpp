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
            ClearPrint();
            changeScene(SceneState::Title, 0);
        }
    }

    void Boot::draw() const {
        Scene::Rect().draw(Palette::Black);
    }
}
