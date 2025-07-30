#pragma once
#include <Siv3D.hpp>

class LoadingAnimationAddon : public IAddon {
public:

    static constexpr StringView Name = U"LoadingAnimationAddon";

    inline static void Begin(const Circle& circle, double thickness, const ColorF& color) {
        if (auto p = Addon::GetAddon<LoadingAnimationAddon>(Name)) {
            p->begin(circle, thickness, color);
        }
    }

    inline static void End() {
        if (auto p = Addon::GetAddon<LoadingAnimationAddon>(Name)) {
            p->end();
        }
    }

    [[nodiscard]]
    inline static bool IsActive() {
        if (auto p = Addon::GetAddon<LoadingAnimationAddon>(Name)) {
            return p->m_active;
        }
        return false;
    }

private:

    static constexpr double LifeTime = 1.5;
    static constexpr double UpdateInterval = 1.0 / 120.0;
    static constexpr double AngleStep = 1.6_deg;

    Circle m_circle { 0, 0, 0 };
    double m_thickness = 0.0;
    ColorF m_color = Palette::White;
    Trail m_trail;
    double m_accumulated_time = 0.0;
    double m_theta = 180_deg;
    bool m_active = false;

    bool init() override;

    bool update() override;

    void draw() const override;

    void begin(const Circle& circle, double thickness, const ColorF& color);

    void end();

    void prewarm();
};
