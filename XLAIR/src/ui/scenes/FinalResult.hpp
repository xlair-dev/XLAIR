#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"
#include "core/features/ApiCall.hpp"

using app::App;

namespace ui {
    class FinalResult : public App::Scene {
    public:
        FinalResult(const InitData& init);
        ~FinalResult();
        void update() override;
        void draw() const override;
    private:
        core::features::ApiCall<bool> m_call;
        bool m_loading = false;
        bool m_synced = false;
    };
}
