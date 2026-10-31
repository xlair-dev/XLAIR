#pragma once

#include "Common.hpp"
#include "HitEffects.hpp"

#include <Playfield/ChartProjection.hpp>
#include <SheetsAnalyzer/Chart.hpp>

namespace xlair::core::judgement {
    class JudgeEngine;
}

namespace xlair::ui::game {
    class PlayfieldRenderer3D {
    public:
        static constexpr double FieldWidth = 18.0;
        static constexpr double FieldFar = 129.2447;
        // The judgement line is 100 / 4000 of the field length from the near edge.
        static constexpr double FieldNear = -FieldFar * 100.0 / 3900.0;

        PlayfieldRenderer3D();

        void addJudgements(const sheets::Chart& chart, std::span<const core::judgement::JudgementEvent> events);

        void draw(
            const BasicCamera3D& camera,
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            int64 current_sample,
            double note_speed,
            uint32 combo,
            bool show_depth_guides = false,
            const core::judgement::JudgeEngine* judge_engine = nullptr,
            uint32 slider_pressed = 0,
            uint8 side_pressed = 0
        ) const;

    private:
        struct FogParameters {
            Float4 color_and_start;
            Float4 end_and_strength;
        };

        mutable MSRenderTexture m_field_surface;
        mutable MSRenderTexture m_render_texture;
        MSRenderTexture m_side_upper_note_surface;
        MSRenderTexture m_side_lower_note_surface;
        MSRenderTexture m_side_upper_xtap_surface;
        MSRenderTexture m_side_lower_xtap_surface;
        PixelShader m_fog_shader;
        ConstantBuffer<FogParameters> m_fog_parameters;
        HitEffects m_hit_effects;
        mutable Array<double> m_measure_depths;
        mutable Array<playfield::HoldSection> m_hold_sections;
        mutable Array<long double> m_current_positions;
        mutable Array<HoldGlow> m_hold_glows;
    };
}
