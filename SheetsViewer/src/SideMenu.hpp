#pragma once
#include <Siv3D.hpp>
#include "SheetManager.hpp"

class SideMenu {
public:
    SideMenu() = default;
    ~SideMenu() = default;

    void update(const SheetManager& sheet_manager);
    void draw(const SheetManager& sheet_manager) const;

private:
    static constexpr auto SideMenuItems = std::array {
        std::pair{ U"\U000F0EFE", U"ID" },
        std::pair{ U"\U000F0CB8", U"Title" },
        std::pair{ U"\U000F04BA", U"Sort" },
        std::pair{ U"\U000F0803", U"Artist" },
        std::pair{ U"\U000F0770", U"Genre" },
        std::pair{ U"\U000F0E2A", U"Music" },
        std::pair{ U"\U000F021F", U"Jacket" },
        std::pair{ U"\U000F0337", U"URL" },
        std::pair{ U"\U000F05B7", U"Offset" },
        std::pair{ U"\U000F07DA", U"BPM" },
    };

    // UI
    struct UI {
        static constexpr int32 Width = 300;
        static constexpr int32 Height = 720;
        static constexpr int32 Margin = 10;

        static constexpr int32 ItemWidth = Width - Margin * 2;
        static constexpr int32 ItemHeight = 27;
        static constexpr int32 ItemOffset = 50;

        static constexpr int32 ItemIconSize = 20;
        static constexpr int32 ItemLabelSize = 15;
        static constexpr int32 ItemTextSize = 15;

        static constexpr int32 ItemLabelOffset = 25;
        static constexpr int32 ItemTextOffset = 80;

        static constexpr int32 ItemDifficultyOffset = 10;
        static constexpr int32 ItemDifficultyHeight = 25;
        static constexpr int32 ItemDifficultyTextOffset = 70;

        static constexpr Vec2 JacketSize{ 90, 90 };
    };

    Optional<size_t> m_selected_index = none;
    SheetManager::State m_previous_state = SheetManager::State::Unselected;

    std::array<String, 10> m_indexed_metadata;
};
