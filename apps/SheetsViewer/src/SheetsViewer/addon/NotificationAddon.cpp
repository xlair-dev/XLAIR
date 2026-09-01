#include "NotificationAddon.hpp"

#include <algorithm>

namespace xlair::sheets_viewer {
    namespace {
        [[nodiscard]]
        s3d::String
        Ellipsize(const s3d::Font& font, const s3d::StringView text, const double font_size, const double max_width) {
            if (text.isEmpty() || max_width <= 0.0) {
                return U"";
            }
            if (font(text).region(font_size).w <= max_width) {
                return s3d::String{ text };
            }

            constexpr s3d::StringView Ellipsis = U"...";
            std::size_t first = 0;
            std::size_t last = text.size();
            while (first < last) {
                const std::size_t middle = first + (last - first + 1) / 2;
                const s3d::String candidate = s3d::String{ text.substr(0, middle) } + Ellipsis;
                if (font(candidate).region(font_size).w <= max_width) {
                    first = middle;
                } else {
                    last = middle - 1;
                }
            }
            return s3d::String{ text.substr(0, first) } + Ellipsis;
        }

        [[nodiscard]]
        s3d::ColorF IconColor(const NotificationAddon::Style& style, const NotificationAddon::Type type) {
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

    void NotificationAddon::Show(const s3d::StringView message, const Type type) {
        if (auto addon = s3d::Addon::GetAddon<NotificationAddon>(Name)) {
            addon->show(message, type);
        }
    }

    void NotificationAddon::SetLifeTime(const double life_time) {
        if (auto addon = s3d::Addon::GetAddon<NotificationAddon>(Name)) {
            addon->m_life_time = std::max(0.5, life_time);
        }
    }

    void NotificationAddon::SetStyle(const Style& style) {
        if (auto addon = s3d::Addon::GetAddon<NotificationAddon>(Name)) {
            addon->m_style = style;
        }
    }

    bool NotificationAddon::update() {
        const double delta_time = s3d::Scene::DeltaTime();
        for (auto& notification : m_notifications) {
            notification.time += delta_time;
        }
        m_notifications.remove_if([life_time = m_life_time](const Notification& notification) {
            return (life_time < notification.time);
        });

        for (std::size_t index = 0; index < m_notifications.size(); ++index) {
            auto& notification = m_notifications[index];
            notification.current_index = s3d::Math::SmoothDamp(
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
        const auto& font = s3d::SimpleGUI::GetFont();
        const double width = std::clamp(m_style.width, 0.0, std::max(0.0, s3d::Scene::Width() - 20.0));
        for (const auto& notification : m_notifications) {
            double x_scale = 1.0;
            double alpha = 1.0;
            if (notification.time < 0.2) {
                x_scale = alpha = (notification.time / 0.2);
            } else if ((m_life_time - 0.4) < notification.time) {
                alpha = ((m_life_time - notification.time) / 0.4);
            }
            alpha = s3d::EaseOutExpo(std::clamp(alpha, 0.0, 1.0));
            x_scale = s3d::EaseOutExpo(std::clamp(x_scale, 0.0, 1.0));

            s3d::ColorF background_color = m_style.background_color;
            s3d::ColorF frame_color = m_style.frame_color;
            s3d::ColorF text_color = m_style.text_color;
            s3d::ColorF icon_color = IconColor(m_style, notification.type);
            background_color.a *= alpha;
            frame_color.a *= alpha;
            text_color.a *= alpha;
            icon_color.a *= alpha;

            const s3d::RectF rect{
                s3d::Arg::topRight =
                    s3d::Vec2{
                        s3d::Scene::Width() - 10,
                        s3d::SimpleMenuBar::MenuBarHeight + 10 + notification.current_index * 40,
                    },
                width * x_scale,
                35,
            };
            rect.rounded(3).draw(background_color).drawFrame(1, 0, frame_color);

            font(Icons[s3d::FromEnum(notification.type)])
                .draw(18, s3d::Arg::leftCenter = rect.leftCenter().movedBy(8, -1), icon_color);
            const s3d::String message = Ellipsize(font, notification.message, 16, width - 46);
            font(message).draw(16, s3d::Arg::leftCenter = rect.leftCenter().movedBy(32, -1), text_color);
        }
    }

    void NotificationAddon::show(const s3d::StringView message, const Type type) {
        const double current_index = m_notifications.isEmpty() ? 0.0 : (m_notifications.back().current_index + 1.0);
        const double velocity = m_notifications.isEmpty() ? 0.0 : m_notifications.back().velocity;
        m_notifications.push_back(
            {
                .message = s3d::String{ message },
                .time = 0.0,
                .current_index = current_index,
                .velocity = velocity,
                .type = type,
            }
        );
    }
}
