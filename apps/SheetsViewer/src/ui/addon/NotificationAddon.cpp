#include "NotificationAddon.hpp"

#include <algorithm>

namespace xlair::sheets_viewer {
    namespace {
        [[nodiscard]]
        String Ellipsize(const Font& font, const StringView text, const double font_size, const double max_width) {
            if (text.isEmpty() || max_width <= 0.0) {
                return U"";
            }
            if (font(text).region(font_size).w <= max_width) {
                return String{ text };
            }

            constexpr StringView Ellipsis = U"...";
            std::size_t first = 0;
            std::size_t last = text.size();
            while (first < last) {
                const std::size_t middle = first + (last - first + 1) / 2;
                const String candidate = String{ text.substr(0, middle) } + Ellipsis;
                if (font(candidate).region(font_size).w <= max_width) {
                    first = middle;
                } else {
                    last = middle - 1;
                }
            }
            return String{ text.substr(0, first) } + Ellipsis;
        }

        [[nodiscard]]
        ColorF IconColor(const NotificationAddon::Style& style, const NotificationAddon::Type type) {
            switch (type) {
                case NotificationAddon::Type::Information:
                    return style.information_color;
                case NotificationAddon::Type::Success:
                    return style.success_color;
                case NotificationAddon::Type::Warning:
                    return style.warning_color;
                case NotificationAddon::Type::Error:
                    return style.error_color;
            }
            return style.information_color;
        }
    }

    void NotificationAddon::Show(const StringView message, const Type type) {
        if (auto addon = Addon::GetAddon<NotificationAddon>(Name)) {
            addon->show(message, type);
        }
    }

    void NotificationAddon::SetLifeTime(const double life_time) {
        if (auto addon = Addon::GetAddon<NotificationAddon>(Name)) {
            addon->m_life_time = std::max(0.5, life_time);
        }
    }

    void NotificationAddon::SetStyle(const Style& style) {
        if (auto addon = Addon::GetAddon<NotificationAddon>(Name)) {
            addon->m_style = style;
        }
    }

    bool NotificationAddon::update() {
        const double delta_time = Scene::DeltaTime();
        for (auto& notification : m_notifications) {
            notification.time += delta_time;
        }
        m_notifications.remove_if([life_time = m_life_time](const Notification& notification) {
            return (life_time < notification.time);
        });

        for (std::size_t index = 0; index < m_notifications.size(); ++index) {
            auto& notification = m_notifications[index];
            notification.current_index = Math::SmoothDamp(
                notification.current_index,
                static_cast<double>(index),
                notification.velocity,
                0.15,
                9'999.0,
                delta_time
            );
        }
        return true;
    }

    void NotificationAddon::draw() const {
        const auto& font = SimpleGUI::GetFont();
        const double width = std::clamp(m_style.width, 0.0, std::max(0.0, Scene::Width() - 20.0));
        for (const auto& notification : m_notifications) {
            double x_scale = 1.0;
            double alpha = 1.0;
            if (notification.time < 0.2) {
                x_scale = alpha = (notification.time / 0.2);
            } else if ((m_life_time - 0.4) < notification.time) {
                alpha = ((m_life_time - notification.time) / 0.4);
            }
            alpha = EaseOutExpo(std::clamp(alpha, 0.0, 1.0));
            x_scale = EaseOutExpo(std::clamp(x_scale, 0.0, 1.0));

            ColorF background_color = m_style.background_color;
            ColorF frame_color = m_style.frame_color;
            ColorF text_color = m_style.text_color;
            ColorF icon_color = IconColor(m_style, notification.type);
            background_color.a *= alpha;
            frame_color.a *= alpha;
            text_color.a *= alpha;
            icon_color.a *= alpha;

            const RectF rect{
                Arg::topRight =
                    Vec2{
                        Scene::Width() - 10,
                        SimpleMenuBar::MenuBarHeight + 10 + notification.current_index * 40,
                    },
                width * x_scale,
                35,
            };
            rect.rounded(3).draw(background_color).drawFrame(1, 0, frame_color);

            font(Icons[FromEnum(notification.type)])
                .draw(18, Arg::leftCenter = rect.leftCenter().movedBy(8, -1), icon_color);
            const String message = Ellipsize(font, notification.message, 16, width - 46);
            font(message).draw(16, Arg::leftCenter = rect.leftCenter().movedBy(32, -1), text_color);
        }
    }

    void NotificationAddon::show(const StringView message, const Type type) {
        const double current_index = m_notifications.isEmpty() ? 0.0 : (m_notifications.back().current_index + 1.0);
        const double velocity = m_notifications.isEmpty() ? 0.0 : m_notifications.back().velocity;
        m_notifications.push_back(
            {
                .message = String{ message },
                .time = 0.0,
                .current_index = current_index,
                .velocity = velocity,
                .type = type,
            }
        );
    }
}
