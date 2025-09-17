#include "addon/NotificationAddon.hpp"

bool NotificationAddon::update() {
    const double delta_time = Scene::DeltaTime();

    for (auto& notification : m_notifications) {
        notification.time += delta_time;
    }

    m_notifications.remove_if([life_time = m_life_time](const Notification& notification) { return (life_time < notification.time); });

    for (size_t i = 0; i < m_notifications.size(); ++i) {
        auto& notification = m_notifications[i];
        notification.current_index = Math::SmoothDamp(notification.current_index,
            static_cast<double>(i), notification.velocity, 0.15, 9999.0, delta_time);
    }

    return true;
}

void NotificationAddon::draw() const {
    const Font& font = SimpleGUI::GetFont();

    for (const auto& notification : m_notifications) {
        double x_scale = 1.0;
        double alpha = 1.0;

        if (notification.time < 0.2) {
            x_scale = alpha = (notification.time / 0.2);
        } else if ((m_life_time - 0.4) < notification.time) {
            alpha = ((m_life_time - notification.time) / 0.4);
        }

        alpha = EaseOutExpo(alpha);
        x_scale = EaseOutExpo(x_scale);

        ColorF background_color = m_style.background_color;
        background_color.a *= alpha;

        ColorF frame_color = m_style.frame_color;
        frame_color.a *= alpha;

        ColorF text_color = m_style.text_color;
        text_color.a *= alpha;


        const RectF rect{ Arg::topRight = Vec2{ Scene::Width() - 10, (10 + notification.current_index * 40)}, (m_style.width * x_scale), 35 };
        rect.rounded(3).draw(background_color).drawFrame(1, 0, frame_color);

        ColorF color = notification.type == Type::Information ? m_style.information_color
            : notification.type == Type::Success ? m_style.success_color
            : notification.type == Type::Warning ? m_style.warning_color
            : m_style.error_color;
        color.a *= alpha;

        font(Icons[FromEnum(notification.type)]).draw(18, Arg::leftCenter = rect.leftCenter().movedBy(8, -1), color);

        font(notification.message).draw(18, Arg::leftCenter = rect.leftCenter().movedBy(32, -1), text_color);
    }
}

void NotificationAddon::show(const StringView message, const Type type) {
    const double current_index = (m_notifications.empty() ? 0.0 : m_notifications.back().current_index + 1.0);
    const double velocity = (m_notifications.empty() ? 0.0 : m_notifications.back().velocity);

    m_notifications << Notification{
        .message = String {message},
        .time = 0.0,
        .current_index = current_index,
        .velocity = velocity,
        .type = type
    };
}

