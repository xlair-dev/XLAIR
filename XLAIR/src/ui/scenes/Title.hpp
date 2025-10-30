#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"
#include "core/features/ApiCall.hpp"
#include "core/types/UserData.hpp"

using app::App;

namespace ui {

    class Title : public App::Scene {
    public:
        Title(const InitData& init);
        ~Title();
        void update() override;
        void draw() const override;

    private:
        bool m_card_scanning = false;
        bool m_scaned = false;
        bool m_loading = false;

        core::features::ApiCall<core::types::UserData> m_call;
    };

}
