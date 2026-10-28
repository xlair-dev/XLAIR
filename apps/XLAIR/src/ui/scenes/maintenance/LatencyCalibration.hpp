#pragma once

#include "Base.hpp"

#include "app/flows/LatencyCalibration.hpp"

namespace xlair::ui::scenes::maintenance {
    class LatencyCalibration final : public Base {
    public:
        explicit LatencyCalibration(const InitData& init);

        ~LatencyCalibration() override;

        void update() override;
        void draw() const override;

    private:
        void reset();

        app::flows::LatencyCalibration m_latency_calibration;
        Audio m_latency_audio;
        double m_latency_time_ms = 0.0;
        bool m_latency_applied = false;
    };
}
