#include "HomographyAddon.hpp"
#include "app/consts/Scene.hpp"

namespace ui::addons {
    bool HomographyAddon::init() {
        setQuad(Quad{
            Rect{ {0, 0}, app::consts::SceneSize }
        });
        return (m_vs && m_ps);
    }

    void HomographyAddon::Draw(const Quad& quad, const Texture& texture) {
        if (auto p = Addon::GetAddon<HomographyAddon>(HomographyAddon::Name)) {
            p->setQuad(quad);
            p->draw(texture);
        }
    }

    void HomographyAddon::setQuad(const Quad& quad) {
        if (m_quad == quad) {
            return;
        }

        m_quad = quad;

        const Mat3x3 mat = Mat3x3::Homography(quad);
        m_vs_constant = { Float4{ mat._11_12_13, 0 }, Float4{ mat._21_22_23, 0 }, Float4{ mat._31_32_33, 0 } };

        const Mat3x3 inv = mat.inverse();
        m_ps_constant = { Float4{ inv._11_12_13, 0 }, Float4{ inv._21_22_23, 0 }, Float4{ inv._31_32_33, 0 } };
    }

    void HomographyAddon::draw(const Texture& texture) const {
        const ScopedCustomShader2D shader{ m_vs, m_ps };
        const ScopedRenderStates2D sampler{ SamplerState::ClampAniso };
        Graphics2D::SetVSConstantBuffer(1, m_vs_constant);
        Graphics2D::SetPSConstantBuffer(1, m_ps_constant);
        Rect{ 1 }(texture).draw();
    }
}
