#include "LoadingAnimationAddon.hpp"

namespace xlair::sheets_viewer {
    void LoadingAnimationAddon::Begin(const StringView message) {
        if (auto addon = Addon::GetAddon<LoadingAnimationAddon>(Name)) {
            addon->begin(message);
        }
    }

    void LoadingAnimationAddon::SetMessage(const StringView message) {
        if (auto addon = Addon::GetAddon<LoadingAnimationAddon>(Name)) {
            addon->m_message = message;
        }
    }

    void LoadingAnimationAddon::End() {
        if (auto addon = Addon::GetAddon<LoadingAnimationAddon>(Name)) {
            addon->end();
        }
    }

    bool LoadingAnimationAddon::IsActive() {
        if (const auto addon = Addon::GetAddon<LoadingAnimationAddon>(Name)) {
            return addon->m_active;
        }
        return false;
    }

    bool LoadingAnimationAddon::init() {
        m_trail = Trail{ LifeTime,
                         [](double) {
                             return 1.0;
                         },
                         EaseOutExpo };
        return true;
    }

    bool LoadingAnimationAddon::update() {
        if (!m_active) {
            return true;
        }

        m_accumulated_time += Scene::DeltaTime();
        while (UpdateInterval <= m_accumulated_time) {
            m_theta = Math::NormalizeAngle(m_theta + AngleStep);
            const Vec2 position = OffsetCircular{ Scene::Center(), 80, m_theta };
            m_trail.update(UpdateInterval);
            m_trail.add(position, ColorF{ 0.8, 0.9, 1.0 }, 10);
            m_accumulated_time -= UpdateInterval;
        }
        return true;
    }

    void LoadingAnimationAddon::draw() const {
        if (!m_active) {
            return;
        }

        Scene::Rect().draw(ColorF{ 0, 0.55 });
        m_trail.draw();
        SimpleGUI::GetFont()(m_message).draw(18, Arg::topCenter = Scene::Center().movedBy(0, 110), Palette::White);
    }

    void LoadingAnimationAddon::begin(const StringView message) {
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
