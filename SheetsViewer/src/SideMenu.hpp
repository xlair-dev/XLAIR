#pragma once
#include <Siv3D.hpp>
#include "SheetManager.hpp"

class SideMenu {
public:
    SideMenu() = default;
    ~SideMenu() = default;

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
};
