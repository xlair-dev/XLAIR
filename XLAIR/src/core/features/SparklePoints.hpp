#pragma once
#include "Common.hpp"

namespace core::features {
    inline Array<Float2> MakeSparkleUnitVerts(const Vertex2D::IndexType quality) {
        Array<Float2> vertices(quality * 4);

        const float step = (90_degF / quality);
        float rad = 0.0_degF;

        Float2* p_dst0 = vertices.data();
        Float2* p_dst1 = (p_dst0 + quality);
        Float2* p_dst2 = (p_dst0 + quality * 2);
        Float2* p_dst3 = (p_dst0 + quality * 3);

        for (Vertex2D::IndexType i = 0; i < quality; ++i) {
            const auto [s, c] = FastMath::SinCos(rad);
            const float sd = std::pow(s, 5.4f);
            const float cd = (c * c * c * c * c);

            p_dst0->set(cd, sd);
            p_dst1->set(-sd, cd);
            p_dst2->set(-cd, -sd);
            p_dst3->set(sd, -cd);

            ++p_dst0;
            ++p_dst1;
            ++p_dst2;
            ++p_dst3;

            rad += step;
        }

        return std::move(vertices);
    }

    inline Array<TriangleIndex> MakeSparkleUnitTris(const Vertex2D::IndexType quality) {
        const size_t num_triangles = (4 * ((quality / 2) * 2 - 1) + 2);

        Array<TriangleIndex> indices(num_triangles);

        const Vertex2D::IndexType mid_index = (quality / 2);
        const Vertex2D::IndexType full_index = (quality * 4);

        TriangleIndex* p_dst = indices.data();

        for (Vertex2D::IndexType k = 0; k < 4; ++k) {
            const Vertex2D::IndexType base_index = (quality * k);

            for (Vertex2D::IndexType i = 0; i < mid_index; ++i) {
                if (i == 0) {
                    p_dst->i0 = (base_index + i);
                    p_dst->i1 = (base_index + i + 1);
                    p_dst->i2 = ((full_index + base_index - 1) % full_index);
                    ++p_dst;
                }
                else {
                    const Vertex2D::IndexType t0 = (base_index + i);
                    const Vertex2D::IndexType t1 = (base_index + i + 1);
                    const Vertex2D::IndexType t2 = ((full_index + base_index - i) % full_index);
                    const Vertex2D::IndexType t3 = (t2 - 1);

                    p_dst->i0 = t0;
                    p_dst->i1 = t1;
                    p_dst->i2 = t2;
                    ++p_dst;

                    p_dst->i0 = t2;
                    p_dst->i1 = t1;
                    p_dst->i2 = t3;
                    ++p_dst;
                }
            }
        }

        const Vertex2D::IndexType t0 = (mid_index);
        const Vertex2D::IndexType t1 = (mid_index + quality);
        const Vertex2D::IndexType t2 = (mid_index + quality * 3);
        const Vertex2D::IndexType t3 = (mid_index + quality * 2);

        p_dst->i0 = t0;
        p_dst->i1 = t1;
        p_dst->i2 = t2;
        ++p_dst;

        p_dst->i0 = t2;
        p_dst->i1 = t1;
        p_dst->i2 = t3;

        return std::move(indices);
    }
}
