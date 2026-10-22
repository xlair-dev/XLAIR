#pragma once

#include "app/controller/Controller.hpp"

namespace xlair::app::flows {
    class LightTest {
    public:
        enum class Target { Slider, Side };
        enum class Pattern { Off, Red, Green, Blue, White, Sequence, Input };

        explicit LightTest(controller::Controller& controller);
        ~LightTest();

        LightTest(const LightTest&) = delete;
        LightTest& operator=(const LightTest&) = delete;

        void start(Pattern pattern);
        void update(double delta_seconds);
        void changeTarget();
        void stop();

        [[nodiscard]]
        bool supported() const noexcept;

        [[nodiscard]]
        Target target() const noexcept {
            return m_target;
        }

        [[nodiscard]]
        Optional<Pattern> activePattern() const noexcept {
            return m_active_pattern;
        }

        [[nodiscard]]
        const Optional<controller::Error>& error() const noexcept {
            return m_error;
        }

    private:
        controller::LightFrame makeFrame() const;

        controller::Controller& m_controller;
        controller::LightFrame m_original_lights;
        controller::LightCapabilities m_capabilities;
        Target m_target = Target::Slider;
        Optional<Pattern> m_active_pattern;
        Optional<controller::Error> m_error;
        Optional<controller::LightFrame> m_last_frame;
        double m_elapsed = 0.0;
        bool m_restore_needed = false;
    };
}
