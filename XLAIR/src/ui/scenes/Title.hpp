#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"
#include "core/features/ApiCall.hpp"
#include "core/types/UserData.hpp"
#include "core/types/Record.hpp"

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
        bool m_loading_records = false;

        core::features::ApiCall<core::types::UserData> m_call;
        core::features::ApiCall<Array<core::types::Record>> m_call_records;
    };

}
