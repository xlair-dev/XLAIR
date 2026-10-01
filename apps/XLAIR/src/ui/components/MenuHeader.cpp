#include "MenuHeader.hpp"

#include "ui/Design.hpp"
#include "ui/theme/Palette.hpp"

#include <utility>

namespace xlair::ui::components {
    std::unique_ptr<TextureAssetData> MakeMenuHeaderTexture(String title, String subtitle) {
        auto data = std::make_unique<TextureAssetData>();
        data->onLoad = [title = std::move(title),
                        subtitle = std::move(subtitle)](TextureAssetData& asset, const String&) {
            const Font title_font{ 70, Resource(U"ui/fonts/BrunoAce/BrunoAce-Regular.ttf") };
            const Font subtitle_font{ 24, Typeface::CJK_Regular_JP };
            const Rect title_region = title_font(title).region(70).asRect();
            const Rect subtitle_region = subtitle_font(subtitle).region(24).asRect();

            Image image{
                static_cast<std::size_t>(title_region.w),
                static_cast<std::size_t>(title_region.h + subtitle_region.h),
            };
            title_font(title).stamp(image, 0, 0);

            for (int32 x = 0; x < image.width(); ++x) {
                const double t = static_cast<double>(x) / image.width();
                for (int32 y = 0; y < image.height(); ++y) {
                    image[y][x] = Color{ theme::Palette::Pink.lerp(theme::Palette::Cyan, t), image[y][x].a };
                }
            }

            subtitle_font(subtitle).stampAt(image, title_region.w / 2.0, 107, theme::Palette::DimmedPurple);
            asset.texture = Texture{ image };
            return static_cast<bool>(asset.texture);
        };
        return data;
    }

    void DrawMenuHeader(const AssetNameView header) {
        TextureAsset{ header }.drawAt(DesignSize.x / 2.0, 153);

        constexpr double LineWidth = 177;
        constexpr double SideOffset = 397.0;
        constexpr ColorF C0{ theme::Palette::DimmedPurple, 0.00 };
        constexpr ColorF C1{ theme::Palette::Cyan, 0.94 };
        constexpr ColorF C2{ theme::Palette::Purple, 0.05 };
        constexpr ColorF C3{ theme::Palette::Pink, 0.00 };

        const auto draw_bar = [&](const double base, const double direction) {
            const double t0 = base;
            const double t1 = base + direction * (LineWidth * 0.06);
            const double t2 = base + direction * (LineWidth * 0.86);
            const double t3 = base + direction * LineWidth;

            for (const double y : { 130.0, 147.0 }) {
                Line{ t0, y, t1, y }.draw(3, C0, C1);
                Line{ t1, y, t2, y }.draw(3, C1, C2);
                Line{ t2, y, t3, y }.draw(3, C2, C3);
            }
        };

        draw_bar(SideOffset + LineWidth, -1.0);
        draw_bar(DesignSize.x - SideOffset - LineWidth, 1.0);
    }
}
