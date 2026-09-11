#pragma once

#include "Common.hpp"

namespace xlair::sheets_viewer {
    class LoadingAnimationAddon : public IAddon {
    public:
        inline static constexpr StringView Name = U"SheetsViewer.LoadingAnimation";

        static void Begin(StringView message);
        static void SetMessage(StringView message);
        static void End();

        [[nodiscard]]
        static bool IsActive();

    private:
        static constexpr double LifeTime = 1.5;
        static constexpr double UpdateInterval = (1.0 / 120.0);
        static constexpr double AngleStep = 1.6_deg;

        bool init() override;
        bool update() override;
        void draw() const override;
        void begin(StringView message);
        void end();
        void prewarm();

        String m_message;
        Trail m_trail;
        double m_accumulated_time = 0.0;
        double m_theta = 180_deg;
        bool m_active = false;
    };
}
