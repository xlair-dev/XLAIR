#pragma once

#include <Siv3D.hpp>

namespace xlair::sheets_viewer {
    class NotificationAddon : public s3d::IAddon {
    public:
        inline static constexpr s3d::StringView Name = U"SheetsViewer.Notification";

        enum class Type {
            Information,
            Success,
            Warning,
            Error,
        };

        struct Style {
            double width = 380.0;
            s3d::ColorF background_color{ 0.0, 0.82 };
            s3d::ColorF frame_color{ 0.75 };
            s3d::ColorF text_color{ 1.0 };
            s3d::ColorF information_color{ 0.0, 0.72, 0.83 };
            s3d::ColorF success_color{ 0.0, 0.78, 0.33 };
            s3d::ColorF warning_color{ 1.0, 0.57, 0.0 };
            s3d::ColorF error_color{ 1.0, 0.32, 0.32 };
        };

        static void Show(s3d::StringView message, Type type = Type::Information);
        static void SetLifeTime(double life_time);
        static void SetStyle(const Style& style);

    private:
        static constexpr s3d::StringView Icons = U"\U000F02FC\U000F0E1E\U000F0029\U000F1398";

        struct Notification {
            s3d::String message;
            double time = 0.0;
            double current_index = 0.0;
            double velocity = 0.0;
            Type type = Type::Information;
        };

        bool update() override;
        void draw() const override;
        void show(s3d::StringView message, Type type);

        Style m_style;
        s3d::Array<Notification> m_notifications;
        double m_life_time = 5.0;
    };
}
