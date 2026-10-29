#include "TapHitEffects.hpp"

#include <array>
#include <cmath>

namespace xlair::ui::game {
    namespace {
        constexpr std::size_t MaximumEffects = 64;
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
    }

    Optional<TapHit> TapHitForJudgement(const sheets::Chart& chart, const core::judgement::JudgementEvent& event) {
        using core::judgement::Grade;
        using core::judgement::NoteKind;
        if (event.grade == Grade::Miss || event.linked_start) {
            return none;
        }
        if (event.note_kind == NoteKind::Slider && event.note_index < chart.slider_notes.size()) {
            const auto& note = chart.slider_notes[event.note_index];
            if (note.kind == sheets::SliderNoteKind::Tap || note.kind == sheets::SliderNoteKind::XTap) {
                return TapHit{ .lane = note.lane,
                               .xtap = note.kind == sheets::SliderNoteKind::XTap,
                               .grade = event.grade };
            }
        } else if (event.note_kind == NoteKind::Side && event.note_index < chart.side_notes.size()) {
            const auto& note = chart.side_notes[event.note_index];
            return TapHit{ .side_button = note.button,
                           .xtap = note.kind == sheets::SideNoteKind::XTap,
                           .grade = event.grade };
        }
        return none;
    }

    TapHitEffects::TapHitEffects()
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

    void TapHitEffects::add(const TapHit& hit, const TapHitSurface& surface) {
        if (m_effects.num_effects() >= MaximumEffects) {
            return;
        }

        const ColorF color = ColorF{ U"#75D8FF" }.removeSRGBCurve();
        const ColorF accent = ColorF{ U"#C999F5" }.removeSRGBCurve();
        const double strength = hit.grade == core::judgement::Grade::Perfect ? 1.0
                                : hit.grade == core::judgement::Grade::Great ? 0.8
                                                                             : 0.6;
        const Vec3 normal{ surface.rotation * Float3{ 0.0f, 1.0f, 0.0f } };
        const Vec3 tangent{ surface.rotation * Float3{ 1.0f, 0.0f, 0.0f } };
        const double width = Clamp(surface.width * 0.65 + 0.8, 1.4, 6.0);
        const std::size_t count = hit.xtap ? 10 : 6;
        std::array<Particle, 10> particles{};
        for (std::size_t index = 0; index < count; ++index) {
            particles[index] = {
                .origin = surface.center + tangent * Random(-0.35, 0.35) * width,
                .velocity = normal * Random(2.0, 4.5) + tangent * Random(-1.5, 1.5) + Vec3{ 0, 0, Random(0.0, 1.5) },
                .delay = index * 0.005,
                .size = static_cast<float>(Random(0.08, 0.13)),
            };
        }

        m_effects.add(
            [this, surface, normal, color, accent, strength, width, count, particles, xtap = hit.xtap](const double t) {
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
                    const double glow = strength * std::pow(Max(0.0, 1.0 - t / 0.1), 2.0);
                    Plane{ surface.center + normal * 0.02, width * 1.3, width * 0.6 }
                        .draw(surface.rotation, m_glow, color.withA(glow * 0.65));
                    Plane{ surface.center + normal * 0.025, width * 0.8, width * 0.25 }
                        .draw(surface.rotation, m_glow, ColorF{ 1.0, glow });
                }

                for (std::size_t index = 0; index < count; ++index) {
                    const auto& particle = particles[index];
                    const double age = t - particle.delay;
                    if (age < 0.0 || age >= ParticleLifetime) {
                        continue;
                    }
                    const double alpha = strength * std::pow(1.0 - age / ParticleLifetime, 2.0);
                    const Vec3 position = particle.origin + particle.velocity * age - normal * (2.0 * age * age);
                    drawParticle(position, Float2{ particle.size * 0.7f, particle.size * 2.2f }, color.withA(alpha));
                    drawParticle(position, Float2{ particle.size * 0.5f, particle.size * 0.5f }, ColorF{ 1.0, alpha });
                }
                return t < Lifetime;
            }
        );
    }

    void TapHitEffects::draw(const BasicCamera3D& camera) const {
        m_billboard_rotation = camera.billboard(Float3{ 0, 0, 0 });
        const ScopedRenderStates3D translucent{ BlendState::NonPremultiplied, DepthStencilState::DepthTest };
        m_effects.update();
    }

    void TapHitEffects::drawParticle(const Vec3& position, const Float2& size, const ColorF& color) const {
        const Mat4x4 transform = Mat4x4::Scale(Float3{ size.x, size.y, 1.0f }) * m_billboard_rotation *
                                 Mat4x4::Translate(Float3{ position });
        m_particle_quad.draw(transform, m_particle, color);
    }
}
