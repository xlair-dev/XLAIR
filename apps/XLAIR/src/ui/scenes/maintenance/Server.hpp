#pragma once

#include "Common.hpp"

namespace xlair::ui::scenes::maintenance {
    class Server final : public Base {
    public:
        explicit Server(const InitData& init);

        ~Server() override;

        void update() override;
        void draw() const override;

    private:
        void updateOperation();
        bool busy() const;

        std::size_t m_selected_index = 0;
        ValueRepeat m_value_repeat;
        bool m_sync_requested = false;
        bool m_loading_jackets = false;
    };
}
