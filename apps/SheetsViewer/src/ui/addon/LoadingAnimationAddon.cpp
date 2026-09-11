#include "LoadingAnimationAddon.hpp"

namespace xlair::sheets_viewer {
    void LoadingAnimationAddon::Begin(const s3d::StringView message) {
        if (auto addon = s3d::Addon::GetAddon<LoadingAnimationAddon>(Name)) {
            addon->begin(message);
        }
    }

    void LoadingAnimationAddon::SetMessage(const s3d::StringView message) {
        if (auto addon = s3d::Addon::GetAddon<LoadingAnimationAddon>(Name)) {
            addon->m_message = message;
        }
    }

    void LoadingAnimationAddon::End() {
        if (auto addon = s3d::Addon::GetAddon<LoadingAnimationAddon>(Name)) {
            addon->end();
        }
    }

    bool LoadingAnimationAddon::IsActive() {
        if (const auto addon = s3d::Addon::GetAddon<LoadingAnimationAddon>(Name)) {
            return addon->m_active;
        }
        return false;
    }

    bool LoadingAnimationAddon::init() {
        m_trail = s3d::Trail{ LifeTime,
                              [](double) {
                                  return 1.0;
                              },
                              s3d::EaseOutExpo };
        return true;
    }

    bool LoadingAnimationAddon::update() {
        if (!m_active) {
            return true;
        }

        m_accumulated_time += s3d::Scene::DeltaTime();
        while (UpdateInterval <= m_accumulated_time) {
            m_theta = s3d::Math::NormalizeAngle(m_theta + AngleStep);
            const s3d::Vec2 position = s3d::OffsetCircular{ s3d::Scene::Center(), 80, m_theta };
            m_trail.update(UpdateInterval);
            m_trail.add(position, s3d::ColorF{ 0.8, 0.9, 1.0 }, 10);
            m_accumulated_time -= UpdateInterval;
        }
        return true;
    }

    void LoadingAnimationAddon::draw() const {
        if (!m_active) {
            return;
        }

        s3d::Scene::Rect().draw(s3d::ColorF{ 0, 0.55 });
        m_trail.draw();
        s3d::SimpleGUI::GetFont()(m_message)
            .draw(18, s3d::Arg::topCenter = s3d::Scene::Center().movedBy(0, 110), s3d::Palette::White);
    }

    void LoadingAnimationAddon::begin(const s3d::StringView message) {
        m_message = message;
        if (m_active) {
            return;
        }

        m_active = true;
        prewarm();
    }

    void LoadingAnimationAddon::end() {
        m_active = false;
        m_message.clear();
    }

    void LoadingAnimationAddon::prewarm() {
        m_trail.update(LifeTime);
        m_accumulated_time = LifeTime;
        m_theta = 180_deg;
        (void)update();
    }
}
