#include "SparkleShape.hpp"
#include "core/features/SparklePoints.hpp"

namespace ui::primitives {
    Shape2D Sparkle(const Vec2& center, const double a, const double b, const double angle, const double quality_scale) {
        const double major_axis = Max(Math::Abs(a), Math::Abs(b));
        const Vertex2D::IndexType quality = (static_cast<Vertex2D::IndexType>(Clamp((major_axis * quality_scale * 0.044 + 2.5), 2.0, 127.0)) * 2);
        assert(IsEven(quality));

        Array<Float2> vertices = core::features::MakeSparkleUnitVerts(quality);

        // move and rotate
        {
            const auto [s, c] = FastMath::SinCos(static_cast<float>(angle));
            const float af = static_cast<float>(a);
            const float bf = static_cast<float>(b);
            const float cx = static_cast<float>(center.x);
            const float cy = static_cast<float>(center.y);

            Float2* p_dst = vertices.data();
            const Float2* p_dstEnd = (p_dst + vertices.size());

            while (p_dst != p_dstEnd) {
                p_dst->x *= af;
                p_dst->y *= bf;

                const float x = p_dst->x;
                const float y = p_dst->y;

                p_dst->x = (x * c - y * s + cx);
                p_dst->y = (x * s + y * c + cy);

                ++p_dst;
            }
        }

        Array<TriangleIndex> indices = core::features::MakeSparkleUnitTris(quality);

        return { std::move(vertices), std::move(indices) };
    }
}
