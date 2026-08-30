#include <ThirdParty/Catch2/catch.hpp>

#include "formats/sus/Compiler.hpp"

namespace sus = xlair::sheets::formats::sus;

TEST_CASE("SideLong start lanes map to XLAIR side buttons", "[SheetsAnalyzer][SUS][Compiler]") {
    const auto check_mapping = [](const s3d::uint8 lane, const xlair::sheets::SideButton expected) {
        const auto button = sus::detail::SideButtonFromSideLongLane(lane);
        REQUIRE(button.has_value());
        CHECK(*button == expected);
    };

    check_mapping(0, xlair::sheets::SideButton::LeftUpper);
    check_mapping(1, xlair::sheets::SideButton::LeftUpper);
    check_mapping(2, xlair::sheets::SideButton::LeftLower);
    check_mapping(3, xlair::sheets::SideButton::LeftLower);
    check_mapping(12, xlair::sheets::SideButton::RightLower);
    check_mapping(13, xlair::sheets::SideButton::RightLower);
    check_mapping(14, xlair::sheets::SideButton::RightUpper);
    check_mapping(15, xlair::sheets::SideButton::RightUpper);

    for (s3d::uint8 lane = 4; lane <= 11; ++lane) {
        CHECK_FALSE(sus::detail::SideButtonFromSideLongLane(lane).has_value());
    }
    CHECK_FALSE(sus::detail::SideButtonFromSideLongLane(16).has_value());
}
