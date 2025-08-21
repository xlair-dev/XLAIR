#pragma once
#include <Siv3D.hpp>

class NotificationAddon : public IAddon {
public:

    inline static constexpr StringView Name = U"NotificationAddon";

    enum class Type {
        Information,
        Success,
        Warning,
        Error,
    };

    struct Style {
        double width = 300.0;
        ColorF background_color { 0.0, 0.7 };
        ColorF frame_color { 0.75 };
        ColorF text_color { 1.0 };

        ColorF information_color { 0.0, 0.72, 0.83 };
        ColorF success_color { 0.0, 0.78, 0.33 };
        ColorF warning_color { 1.0, 0.57, 0.0 };
        ColorF error_color { 1.00, 0.32, 0.32 };
    };

    inline static void Show(const StringView message, const Type type = Type::Information) {
        if (auto p = Addon::GetAddon<NotificationAddon>(Name)) {
            p->show(message, type);
        }
    }

    inline static void SetLifeTime(const double life_time) {
        if (auto p = Addon::GetAddon<NotificationAddon>(Name)) {
            p->m_life_time = life_time;
        }
    }

    static void SetStyle(const Style& style) {
        if (auto p = Addon::GetAddon<NotificationAddon>(Name)) {
            p->m_style = style;
        }
    }

private:

    static constexpr StringView Icons = U"\U000F02FC\U000F0E1E\U000F0029\U000F1398";

    struct Notification {
        String message;
        double time = 0.0;
        double current_index = 0.0;
        double velocity = 0.0;
        Type type = Type::Information;
    };

    Style m_style;
    Array<Notification> m_notifications;
    double m_life_time = 3.0;

    bool update() override;

    void draw() const override;

    void show(const StringView message, const Type type);
};
