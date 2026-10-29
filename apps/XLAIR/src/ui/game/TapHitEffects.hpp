#pragma once

#include "Common.hpp"
#include "core/judgement/JudgeEngine.hpp"

#include <span>

namespace xlair::ui::game {
    struct TapHit {
        sheets::LaneSpan lane;
        Optional<sheets::SideButton> side_button;
        bool xtap = false;
        core::judgement::Grade grade = core::judgement::Grade::Perfect;
    };

    [[nodiscard]]
    Optional<TapHit> TapHitForJudgement(const sheets::Chart& chart, const core::judgement::JudgementEvent& event);

    struct TapHitSurface {
        Vec3 center{ 0, 0, 0 };
        Quaternion rotation = Quaternion::Identity();
        double width = 1.0;
    };

    class TapHitEffects {
    public:
        TapHitEffects();
        TapHitEffects(const TapHitEffects&) = delete;
        TapHitEffects& operator=(const TapHitEffects&) = delete;

        void add(const TapHit& hit, const TapHitSurface& surface);
        void draw(const BasicCamera3D& camera) const;

    private:
        static constexpr double Lifetime = 0.42;

        void drawParticle(const Vec3& position, const Float2& size, const ColorF& color) const;

        Texture m_ring;
        Texture m_glow;
        Texture m_particle;
        Mesh m_particle_quad;
        mutable Mat4x4 m_billboard_rotation = Mat4x4::Identity();
        Effect m_effects{ Lifetime };
    };
}
