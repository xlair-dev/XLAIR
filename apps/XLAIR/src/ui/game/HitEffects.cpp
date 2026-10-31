#include "HitEffects.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace xlair::ui::game {
    namespace {
        constexpr std::size_t MaximumEffects = 64;
        constexpr std::size_t MaximumHoldEffects = 32;
        constexpr double RingLifetime = 0.24;
        constexpr double ParticleLifetime = 0.36;

        template <class Alpha>
        [[nodiscard]]
        Image MakeMask(const int32 size, Alpha alpha) {
            Image image{ Size{ size, size }, Color{ 255, 255, 255, 0 } };
            for (int32 y = 0; y < size; ++y) {
                for (int32 x = 0; x < size; ++x) {
                    const double u = (x + 0.5) * 2.0 / size - 1.0;
                    const double v = (y + 0.5) * 2.0 / size - 1.0;
                    image[y][x].a = static_cast<uint8>(255.0 * Clamp(alpha(u, v), 0.0, 1.0));
                }
            }
            return image;
        }

        struct Particle {
            Vec3 origin;
            Vec3 velocity;
            double delay = 0.0;
            float size = 0.0f;
        };

        [[nodiscard]]
        EffectLaneSpan ToEffectLane(const sheets::LaneSpan lane) {
            return { static_cast<double>(lane.start), static_cast<double>(lane.width) };
        }

        [[nodiscard]]
        double Strength(const core::judgement::Grade grade) {
            return grade == core::judgement::Grade::Perfect ? 1.0 : grade == core::judgement::Grade::Great ? 0.8 : 0.6;
        }

        [[nodiscard]]
        HitKind HoldKind(const std::size_t index, const std::size_t count) {
            if (index == 0) {
                return HitKind::HoldStart;
            }
            return index + 1 == count ? HitKind::HoldEnd : HitKind::HoldTick;
        }

        [[nodiscard]]
        ColorF HoldColor(const Optional<sheets::SideButton> button) {
            if (!button) {
                return ColorF{ U"#43A4F2" }.removeSRGBCurve();
            }
            const bool upper = *button == sheets::SideButton::LeftUpper || *button == sheets::SideButton::RightUpper;
            return (upper ? ColorF{ U"#8060B8" } : ColorF{ U"#BC269F" }).removeSRGBCurve();
        }
    }

    Optional<Hit> HitForJudgement(const sheets::Chart& chart, const core::judgement::JudgementEvent& event) {
        using core::judgement::Grade;
        using core::judgement::NoteKind;
        if (event.grade == Grade::Miss || event.linked_start) {
            return none;
        }
        if (event.note_kind == NoteKind::Slider && event.note_index < chart.slider_notes.size()) {
            const auto& note = chart.slider_notes[event.note_index];
            HitKind kind = HitKind::Tap;
            switch (note.kind) {
                case sheets::SliderNoteKind::Tap:
                    break;
                case sheets::SliderNoteKind::XTap:
                    kind = HitKind::XTap;
                    break;
                case sheets::SliderNoteKind::Flick:
                    kind = HitKind::Flick;
                    break;
            }
            return Hit{ .lane = ToEffectLane(note.lane), .kind = kind, .grade = event.grade };
        } else if (event.note_kind == NoteKind::Side && event.note_index < chart.side_notes.size()) {
            const auto& note = chart.side_notes[event.note_index];
            return Hit{ .side_button = note.button,
                        .kind = note.kind == sheets::SideNoteKind::XTap ? HitKind::XTap : HitKind::Tap,
                        .grade = event.grade };
        } else if (event.note_kind == NoteKind::SliderHold && event.note_index < chart.slider_holds.size()) {
            const auto& points = chart.slider_holds[event.note_index].judge_points;
            if (event.point_index >= points.size()) {
                return none;
            }
            const auto& lane = points[event.point_index].lane;
            return Hit{ .lane = ToEffectLane(lane),
                        .kind = HoldKind(event.point_index, points.size()),
                        .grade = event.grade };
        } else if (event.note_kind == NoteKind::SideHold && event.note_index < chart.side_holds.size()) {
            const auto& hold = chart.side_holds[event.note_index];
            if (event.point_index >= hold.judge_samples.size()) {
                return none;
            }
            return Hit{ .side_button = hold.button,
                        .kind = HoldKind(event.point_index, hold.judge_samples.size()),
                        .grade = event.grade };
        }
        return none;
    }

    Optional<EffectLaneSpan> EvaluateEffectHoldLane(const Array<sheets::SliderHoldPoint>& points, const int64 sample) {
        if (points.isEmpty()) {
            return none;
        }
        const auto next =
            std::upper_bound(points.begin(), points.end(), sample, [](const int64 value, const auto& point) {
                return value < point.sample;
            });
        if (next == points.begin()) {
            return ToEffectLane(points.front().lane);
        }
        const auto& before = *std::prev(next);
        if (next == points.end()) {
            return ToEffectLane(before.lane);
        }
        const double t = static_cast<double>(
            (static_cast<long double>(sample) - before.sample) /
            (static_cast<long double>(next->sample) - before.sample)
        );
        return EffectLaneSpan{
            Math::Lerp(static_cast<double>(before.lane.start), static_cast<double>(next->lane.start), t),
            Math::Lerp(static_cast<double>(before.lane.width), static_cast<double>(next->lane.width), t),
        };
    }

    HitEffects::HitEffects()
        : m_ring{ MakeMask(
                      128,
                      [](const double u, const double v) {
                          const double distance = std::hypot(u, v) - 0.7;
                          return std::exp(-distance * distance / 0.0008) + 0.2 * std::exp(-distance * distance / 0.008);
                      }
                  ),
                  TextureDesc::Mipped },
          m_glow{ MakeMask(
                      64,
                      [](const double u, const double v) {
                          return std::pow(Max(0.0, 1.0 - std::hypot(u, v)), 2.0);
                      }
                  ),
                  TextureDesc::Mipped },
          m_particle{ MakeMask(
                          32,
                          [](const double u, const double v) {
                              return std::pow(Max(0.0, 1.0 - std::abs(u) - std::abs(v)), 0.6);
                          }
                      ),
                      TextureDesc::Mipped },
          m_particle_quad{ MeshData::Billboard() } {}

    void HitEffects::add(const Hit& hit, const HitSurface& surface) {
        if (hit.grade == core::judgement::Grade::Miss) {
            return;
        }
        if (hit.kind == HitKind::HoldStart || hit.kind == HitKind::HoldTick || hit.kind == HitKind::HoldEnd) {
            addHold(hit, surface);
            return;
        }
        if (m_effects.num_effects() >= MaximumEffects) {
            return;
        }

        if (hit.kind == HitKind::Flick) {
            addFlick(hit, surface);
            return;
        }

        const bool xtap = hit.kind == HitKind::XTap;
        const ColorF color =
            (xtap ? (hit.side_button ? ColorF{ U"#F4B900" } : ColorF{ U"#FEDF64" }) : ColorF{ U"#75D8FF" })
                .removeSRGBCurve();
        const ColorF accent = (hit.side_button ? ColorF{ U"#FFECA0" } : ColorF{ U"#FEEFB4" }).removeSRGBCurve();
        const double strength = Strength(hit.grade);
        const Vec3 normal{ surface.rotation * Float3{ 0.0f, 1.0f, 0.0f } };
        const Vec3 tangent{ surface.rotation * Float3{ 1.0f, 0.0f, 0.0f } };
        const double width = Clamp(surface.width * 0.65 + 0.8, 1.4, 6.0);
        const std::size_t count = xtap ? 10 : 6;
        std::array<Particle, 10> particles{};
        for (std::size_t index = 0; index < count; ++index) {
            particles[index] = {
                .origin = surface.center + tangent * Random(-0.35, 0.35) * width,
                .velocity = normal * Random(2.0, 4.5) + tangent * Random(-1.5, 1.5) + Vec3{ 0, 0, Random(0.0, 1.5) },
                .delay = index * 0.005,
                .size = static_cast<float>(Random(0.08, 0.13)),
            };
        }

        m_effects.add([this, surface, normal, color, accent, strength, width, count, particles, xtap](const double t) {
            if (t < RingLifetime) {
                const double progress = t / RingLifetime;
                const double expansion = 0.55 + 0.75 * EaseOutCubic(progress);
                const double alpha = strength * (1.0 - progress) * (1.0 - progress);
                Plane{ surface.center, width * expansion, width * expansion * 0.7 }
                    .draw(surface.rotation, m_ring, color.withA(alpha));
                if (xtap) {
                    Plane{ surface.center + normal * 0.015, width * expansion * 0.75, width * expansion * 0.5 }
                        .draw(surface.rotation, m_ring, accent.withA(alpha * 0.8));
                }
                const double glow_fade = Max(0.0, 1.0 - t / 0.1);
                const double glow = strength * (glow_fade * glow_fade);
                if (glow > 0.0) {
                    Plane{ surface.center + normal * 0.02, width * 1.3, width * 0.6 }
                        .draw(surface.rotation, m_glow, color.withA(glow * 0.65));
                    Plane{ surface.center + normal * 0.025, width * 0.8, width * 0.25 }
                        .draw(surface.rotation, m_glow, ColorF{ 1.0, glow });
                }
            }

            for (std::size_t index = 0; index < count; ++index) {
                const auto& particle = particles[index];
                const double age = t - particle.delay;
                if (age < 0.0 || age >= ParticleLifetime) {
                    continue;
                }
                const double fade = 1.0 - age / ParticleLifetime;
                const double alpha = strength * (fade * fade);
                const Vec3 position = particle.origin + particle.velocity * age - normal * (2.0 * age * age);
                drawParticle(position, Float2{ particle.size * 0.7f, particle.size * 2.2f }, color.withA(alpha));
                drawParticle(position, Float2{ particle.size * 0.5f, particle.size * 0.5f }, ColorF{ 1.0, alpha });
            }
            return t < Lifetime;
        });
    }

    void HitEffects::addFlick(const Hit& hit, const HitSurface& surface) {
        const ColorF color = ColorF{ U"#B429FA" }.removeSRGBCurve();
        const Vec3 normal{ surface.rotation * Float3{ 0, 1, 0 } };
        const Vec3 tangent{ surface.rotation * Float3{ 1, 0, 0 } };
        const double width = Clamp(surface.width * 0.65 + 0.8, 1.4, 6.0);
        const double strength = Strength(hit.grade);
        const bool completed = hit.grade != core::judgement::Grade::Good;
        m_effects.add([this, surface, normal, tangent, color, width, strength, completed](const double t) {
            constexpr double Duration = 0.28;
            if (t >= Duration) {
                return false;
            }
            const double progress = t / Duration;
            const double fade = 1.0 - progress;
            const double alpha = strength * fade * fade;
            const double reach = width * (completed ? (0.15 + 0.55 * EaseOutCubic(progress)) : 0.1);
            for (const double side : { -1.0, 1.0 }) {
                const Vec3 position = surface.center + tangent * (side * reach) + normal * 0.02;
                Plane{ position, width * (completed ? 0.85 : 0.5), 0.32 * (1.0 - progress * 0.5) }
                    .draw(surface.rotation, m_glow, color.withA(alpha));
                if (completed) {
                    drawParticle(position + normal * (progress * 0.8), Float2{ 0.12f, 0.22f }, ColorF{ 1.0, alpha });
                }
            }
            return true;
        });
    }

    void HitEffects::addHold(const Hit& hit, const HitSurface& surface) {
        if (m_hold_effects.num_effects() >= MaximumHoldEffects) {
            return;
        }
        const ColorF color = HoldColor(hit.side_button);
        const Vec3 normal{ surface.rotation * Float3{ 0, 1, 0 } };
        const double width = Clamp(surface.width * 0.8, 0.8, 6.0);
        const double strength = Strength(hit.grade);
        const HitKind kind = hit.kind;
        m_hold_effects.add([this, surface, normal, color, width, strength, kind](const double t) {
            const bool tick = kind == HitKind::HoldTick;
            const double duration = tick ? 0.14 : HoldLifetime;
            if (t >= duration) {
                return false;
            }
            const double progress = t / duration;
            const double fade = 1.0 - progress;
            const double alpha = strength * fade * fade;
            const double scale = kind == HitKind::HoldEnd ? 1.0 - progress * 0.35 : 0.65 + progress * 0.65;
            Plane{ surface.center + normal * 0.02, width * scale, tick ? 0.35 : 0.6 }
                .draw(surface.rotation, m_glow, color.withA(alpha * (tick ? 0.7 : 0.9)));
            Plane{ surface.center + normal * 0.025, width * scale * 0.8, tick ? 0.10 : 0.16 }
                .draw(surface.rotation, m_glow, ColorF{ 1.0, alpha * 0.8 });
            if (!tick) {
                for (const double side : { -1.0, 1.0 }) {
                    const Vec3 offset{ surface.rotation * Float3{ static_cast<float>(side * width * 0.25), 0, 0 } };
                    drawParticle(
                        surface.center + offset + normal * (0.05 + progress),
                        Float2{ 0.09f, 0.18f },
                        color.withA(alpha)
                    );
                }
            }
            return true;
        });
    }

    void HitEffects::drawHold(const HoldGlow& hold, const double seconds) const {
        const auto& surface = hold.surface;
        const ColorF color = HoldColor(hold.side_button);
        const Vec3 normal{ surface.rotation * Float3{ 0, 1, 0 } };
        const Vec3 tangent{ surface.rotation * Float3{ 1, 0, 0 } };
        const double pulse = 0.65 + 0.1 * Math::Sin(seconds * Math::TwoPi * 4.0);
        Plane{ surface.center + normal * 0.02, surface.width * 0.95, 0.65 }
            .draw(surface.rotation, m_glow, color.withA(pulse));
        Plane{ surface.center + normal * 0.025, surface.width * 0.8, 0.13 }
            .draw(surface.rotation, m_glow, ColorF{ 1.0, 0.7 });
        const ColorF particle_color = Math::Lerp(color, ColorF{ 1.0 }, hold.side_button ? 0.12 : 0.45);
        const double spread = Clamp(surface.width, 0.8, 6.0);
        constexpr double ParticleDuration = 0.5;
        for (int32 index = 0; index < 2; ++index) {
            const double cycle = seconds / ParticleDuration + index * 0.5;
            const double progress = cycle - std::floor(cycle);
            const double age = progress * ParticleDuration;
            const double alpha = Min(progress * 12.0, 1.0) * (1.0 - progress * progress);
            const double side = index == 0 ? -1.0 : 1.0;
            const double height = 0.05 + 6.5 * age - 5.5 * age * age;
            const Vec3 position =
                surface.center + tangent * (side * spread * (0.12 + progress * 0.25)) + normal * height;
            const float scale = static_cast<float>(1.0 - progress * 0.35);
            drawParticle(position, Float2{ 0.16f * scale, 0.30f * scale }, particle_color.withA(alpha));
        }
    }

    void
    HitEffects::draw(const BasicCamera3D& camera, const std::span<const HoldGlow> holds, const double seconds) const {
        m_billboard_rotation = camera.billboard(Float3{ 0, 0, 0 });
        const ScopedRenderStates3D translucent{ BlendState::NonPremultiplied, DepthStencilState::DepthTest };
        for (const auto& hold : holds) {
            drawHold(hold, seconds);
        }
        m_hold_effects.update();
        m_effects.update();
    }

    void HitEffects::drawParticle(const Vec3& position, const Float2& size, const ColorF& color) const {
        const Mat4x4 transform = Mat4x4::Scale(Float3{ size.x, size.y, 1.0f }) * m_billboard_rotation *
                                 Mat4x4::Translate(Float3{ position });
        m_particle_quad.draw(transform, m_particle, color);
    }
}
