#pragma once

#include "Common.hpp"
#include "core/judgement/JudgeEngine.hpp"

#include <span>

namespace xlair::ui::game {
    enum class HitKind {
        Tap,
        XTap,
        Flick,
        HoldStart,
        HoldTick,
        HoldEnd,
    };

    struct EffectLaneSpan {
        double start = 0.0;
        double width = 1.0;
    };

    struct Hit {
        EffectLaneSpan lane;
        Optional<sheets::SideButton> side_button;
        HitKind kind = HitKind::Tap;
        core::judgement::Grade grade = core::judgement::Grade::Perfect;
    };

    [[nodiscard]]
    Optional<Hit> HitForJudgement(const sheets::Chart& chart, const core::judgement::JudgementEvent& event);

    // Keep visual motion continuous instead of rounding to the covered input lanes.
    [[nodiscard]]
    Optional<EffectLaneSpan> EvaluateEffectHoldLane(const Array<sheets::SliderHoldPoint>& points, int64 sample);

    struct HitSurface {
        Vec3 center{ 0, 0, 0 };
        Quaternion rotation = Quaternion::Identity();
        double width = 1.0;
    };

    struct HoldGlow {
        HitSurface surface;
        Optional<sheets::SideButton> side_button;
    };

    class HitEffects {
    public:
        HitEffects();
        HitEffects(const HitEffects&) = delete;
        HitEffects& operator=(const HitEffects&) = delete;

        void add(const Hit& hit, const HitSurface& surface);
        void draw(const BasicCamera3D& camera, std::span<const HoldGlow> holds, double seconds) const;

    private:
        static constexpr double Lifetime = 0.42;
        static constexpr double HoldLifetime = 0.24;

        void addFlick(const Hit& hit, const HitSurface& surface);
        void addHold(const Hit& hit, const HitSurface& surface);
        void drawHold(const HoldGlow& hold, double seconds) const;
        void drawParticle(const Vec3& position, const Float2& size, const ColorF& color) const;

        Texture m_ring;
        Texture m_glow;
        Texture m_particle;
        Mesh m_particle_quad;
        mutable Mat4x4 m_billboard_rotation = Mat4x4::Identity();
        Effect m_effects{ Lifetime };
        Effect m_hold_effects{ HoldLifetime };
    };
}
