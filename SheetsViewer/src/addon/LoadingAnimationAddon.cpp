#include "LoadingAnimationAddon.hpp"

bool LoadingAnimationAddon::init() {
    m_trail = Trail{ LifeTime, [](double) { return 1.0; }, EaseOutExpo };
    return true;
}

bool LoadingAnimationAddon::update() {
    if (not m_active) {
        return true;
    }

    m_accumulated_time += Scene::DeltaTime();

    while (UpdateInterval <= m_accumulated_time) {
        m_theta = Math::NormalizeAngle(m_theta + AngleStep);
        const Vec2 pos = OffsetCircular{ m_circle.center, m_circle.r, m_theta };

        m_trail.update(UpdateInterval);
        m_trail.add(pos, m_color, m_thickness);
        m_accumulated_time -= UpdateInterval;
    }

    return true;
}

void LoadingAnimationAddon::draw() const {
    if (not m_active) {
        return;
    }
    Scene::Rect().draw(ColorF{ 0, 0.5 });
    m_trail.draw();
}

void LoadingAnimationAddon::begin(const Circle& circle, double thickness, const ColorF& color) {
    m_circle = circle;
    m_thickness = thickness;
    m_color = color;
    m_active = true;
    prewarm();
}

void LoadingAnimationAddon::end() {
    m_active = false;
}

void LoadingAnimationAddon::prewarm() {
    m_trail.update(LifeTime);
    m_accumulated_time = LifeTime;
    m_theta = 180_deg;

    update();
}
