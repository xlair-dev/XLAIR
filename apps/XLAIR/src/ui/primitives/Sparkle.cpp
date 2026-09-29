#include "Sparkle.hpp"

#include <cassert>
#include <cmath>
#include <utility>

namespace xlair::ui::primitives {
    namespace {
        Array<Float2> MakeUnitVertices(const Vertex2D::IndexType quality) {
            Array<Float2> vertices(quality * 4);

            const float step = (90_degF / quality);
            float angle = 0.0_degF;

            Float2* destination_0 = vertices.data();
            Float2* destination_1 = destination_0 + quality;
            Float2* destination_2 = destination_0 + quality * 2;
            Float2* destination_3 = destination_0 + quality * 3;

            for (Vertex2D::IndexType index = 0; index < quality; ++index) {
                const auto [sin, cos] = FastMath::SinCos(angle);
                const float sin_distance = std::pow(sin, 5.4f);
                const float cos_distance = cos * cos * cos * cos * cos;

                destination_0->set(cos_distance, sin_distance);
                destination_1->set(-sin_distance, cos_distance);
                destination_2->set(-cos_distance, -sin_distance);
                destination_3->set(sin_distance, -cos_distance);

                ++destination_0;
                ++destination_1;
                ++destination_2;
                ++destination_3;
                angle += step;
            }

            return vertices;
        }

        Array<TriangleIndex> MakeTriangleIndices(const Vertex2D::IndexType quality) {
            const std::size_t triangle_count = 4 * ((quality / 2) * 2 - 1) + 2;
            Array<TriangleIndex> indices(triangle_count);

            const Vertex2D::IndexType middle = quality / 2;
            const Vertex2D::IndexType vertex_count = quality * 4;
            TriangleIndex* destination = indices.data();

            for (Vertex2D::IndexType quadrant = 0; quadrant < 4; ++quadrant) {
                const Vertex2D::IndexType base = quality * quadrant;

                for (Vertex2D::IndexType index = 0; index < middle; ++index) {
                    if (index == 0) {
                        destination->i0 = base;
                        destination->i1 = base + 1;
                        destination->i2 = (vertex_count + base - 1) % vertex_count;
                        ++destination;
                        continue;
                    }

                    const Vertex2D::IndexType index_0 = base + index;
                    const Vertex2D::IndexType index_1 = base + index + 1;
                    const Vertex2D::IndexType index_2 = (vertex_count + base - index) % vertex_count;
                    const Vertex2D::IndexType index_3 = index_2 - 1;

                    destination->i0 = index_0;
                    destination->i1 = index_1;
                    destination->i2 = index_2;
                    ++destination;

                    destination->i0 = index_2;
                    destination->i1 = index_1;
                    destination->i2 = index_3;
                    ++destination;
                }
            }

            const Vertex2D::IndexType index_0 = middle;
            const Vertex2D::IndexType index_1 = middle + quality;
            const Vertex2D::IndexType index_2 = middle + quality * 3;
            const Vertex2D::IndexType index_3 = middle + quality * 2;

            destination->i0 = index_0;
            destination->i1 = index_1;
            destination->i2 = index_2;
            ++destination;

            destination->i0 = index_2;
            destination->i1 = index_1;
            destination->i2 = index_3;

            return indices;
        }
    }

    Shape2D Sparkle(
        const Vec2& center,
        const double horizontal_radius,
        const double vertical_radius,
        const double angle,
        const double quality_scale
    ) {
        const double major_radius = Max(Math::Abs(horizontal_radius), Math::Abs(vertical_radius));
        const auto quality =
            static_cast<Vertex2D::IndexType>(Math::Clamp(major_radius * quality_scale * 0.044 + 2.5, 2.0, 127.0)) * 2;
        assert(IsEven(quality));

        Array<Float2> vertices = MakeUnitVertices(quality);
        const auto [sin, cos] = FastMath::SinCos(static_cast<float>(angle));
        const float horizontal = static_cast<float>(horizontal_radius);
        const float vertical = static_cast<float>(vertical_radius);
        const float center_x = static_cast<float>(center.x);
        const float center_y = static_cast<float>(center.y);

        for (auto& vertex : vertices) {
            vertex.x *= horizontal;
            vertex.y *= vertical;

            const float x = vertex.x;
            const float y = vertex.y;
            vertex.x = x * cos - y * sin + center_x;
            vertex.y = x * sin + y * cos + center_y;
        }

        return Shape2D{ std::move(vertices), MakeTriangleIndices(quality) };
    }
}
