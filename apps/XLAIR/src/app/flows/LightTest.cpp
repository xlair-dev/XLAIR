#include "LightTest.hpp"

#include <cmath>

namespace xlair::app::flows {
    namespace {
        constexpr double SequenceInterval = 0.12;

        Color PatternColor(const LightTest::Pattern pattern) {
            switch (pattern) {
                case LightTest::Pattern::Off:
                    return Palette::Black;
                case LightTest::Pattern::Red:
                    return Palette::Red;
                case LightTest::Pattern::Green:
                    return Color{ 0, 255, 0 };
                case LightTest::Pattern::Blue:
                    return Palette::Blue;
                default:
                    return Palette::White;
            }
        }
    }

    LightTest::LightTest(controller::Controller& controller)
        : m_controller{ controller }, m_original_lights{ controller.lights() },
          m_capabilities{ controller.lightCapabilities() },
          m_target{ m_capabilities.slider ? Target::Slider : Target::Side } {}

    LightTest::~LightTest() {
        stop();
    }

    bool LightTest::supported() const noexcept {
        return m_target == Target::Slider ? m_capabilities.slider : m_capabilities.side;
    }

    void LightTest::start(const Pattern pattern) {
        m_error.reset();
        if (!supported()) {
            m_error = controller::Error{ controller::ErrorKind::Unavailable, U"LED output is not supported." };
            return;
        }
        m_active_pattern = pattern;
        m_elapsed = 0.0;
        update(0.0);
    }

    void LightTest::update(const double delta_seconds) {
        if (!m_active_pattern) {
            return;
        }
        if (std::isfinite(delta_seconds) && delta_seconds > 0.0) {
            const auto count = m_target == Target::Slider ? controller::SliderLightCount : controller::SideButtonCount;
            m_elapsed = std::fmod(m_elapsed + delta_seconds, count * SequenceInterval);
        }
        const auto frame = makeFrame();
        if (m_last_frame && *m_last_frame == frame) {
            return;
        }
        // A failed write may still have changed the physical lights; restore on exit in either case.
        m_restore_needed = true;
        if (const auto result = m_controller.setLights(frame); !result) {
            m_error = result.error;
            m_active_pattern.reset();
            m_last_frame.reset();
            return;
        }
        m_last_frame = frame;
    }

    void LightTest::changeTarget() {
        if (!m_capabilities.slider || !m_capabilities.side) {
            return;
        }
        stop();
        if (!m_restore_needed) {
            m_target = m_target == Target::Slider ? Target::Side : Target::Slider;
        }
    }

    void LightTest::stop() {
        m_active_pattern.reset();
        m_last_frame.reset();
        if (!m_restore_needed) {
            return;
        }
        if (const auto result = m_controller.setLights(m_original_lights); !result) {
            m_error = result.error;
            Logger << U"[Maintenance] Failed to restore controller lights: " + m_error->message;
            return;
        }
        m_restore_needed = false;
    }

    controller::LightFrame LightTest::makeFrame() const {
        auto frame = m_original_lights;
        const auto pattern = *m_active_pattern;
        const auto color = PatternColor(pattern);
        if (m_target == Target::Slider) {
            frame.slider.fill(Palette::Black);
            if (pattern == Pattern::Sequence) {
                frame.slider[static_cast<std::size_t>(m_elapsed / SequenceInterval) % frame.slider.size()] = color;
            } else if (pattern == Pattern::Input) {
                // Map each touch column to its LED and the adjacent separators.
                const auto pressed = [&](const std::size_t column) {
                    return m_controller.touchZone(column * 2).pressed() ||
                           m_controller.touchZone(column * 2 + 1).pressed();
                };
                for (std::size_t index = 0; index < frame.slider.size(); ++index) {
                    const auto column = index / 2;
                    if (pressed(column) || (index % 2 != 0 && pressed(column + 1))) {
                        frame.slider[index] = color;
                    }
                }
            } else {
                frame.slider.fill(color);
            }
        } else {
            frame.side.fill(Palette::Black);
            if (pattern == Pattern::Sequence) {
                constexpr std::array Order{
                    controller::SideButton::LeftUpper,
                    controller::SideButton::LeftLower,
                    controller::SideButton::RightLower,
                    controller::SideButton::RightUpper,
                };
                const auto index = static_cast<std::size_t>(m_elapsed / SequenceInterval) % Order.size();
                frame.side[static_cast<std::size_t>(Order[index])] = color;
            } else if (pattern == Pattern::Input) {
                for (std::size_t index = 0; index < frame.side.size(); ++index) {
                    if (m_controller.sideButton(static_cast<controller::SideButton>(index)).pressed()) {
                        frame.side[index] = color;
                    }
                }
            } else {
                frame.side.fill(color);
            }
        }
        return frame;
    }
}
