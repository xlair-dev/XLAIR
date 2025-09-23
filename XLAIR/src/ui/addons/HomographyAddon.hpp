#pragma once
#include "Common.hpp"

namespace ui::addons {
    class HomographyAddon : public IAddon {
    public:
        inline static constexpr StringView Name{ U"SheetManagerAddon" };

        static void Draw(const Quad& quad, const Texture& texture);

    private:
        bool init() override;
        void setQuad(const s3d::Quad& quad);
        void draw(const Texture& texture) const;

        struct Homography {
            Float4 m1;
            Float4 m2;
            Float4 m3;
        };

        VertexShader m_vs = HLSL{ U"assets/shaders/homography.hlsl", U"VS" } | GLSL{ U"assets/shaders/homography.vert", { { U"VSConstants2D", 0 }, { U"VSHomography", 1 } } };
        PixelShader m_ps = HLSL{ U"assets/shaders/homography.hlsl", U"PS" } | GLSL{ U"assets/shaders/homography.frag", { { U"PSConstants2D", 0 }, { U"PSHomography", 1 } } };

        ConstantBuffer<Homography> m_vs_constant;
        ConstantBuffer<Homography> m_ps_constant;
        Quad m_quad{};
    };
}
