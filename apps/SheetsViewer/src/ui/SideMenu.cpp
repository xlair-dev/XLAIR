#include "SideMenu.hpp"

#include <array>

namespace xlair::sheets_viewer {
    namespace {
        constexpr int32 Margin = 10;
        constexpr int32 ItemHeight = 27;
        constexpr int32 IconSize = 20;
        constexpr int32 LabelSize = 15;
        constexpr int32 TextSize = 15;
        constexpr int32 LabelOffset = 25;
        constexpr int32 TextOffset = 80;
        constexpr int32 DifficultyHeight = 28;

        constexpr auto MetadataItems = std::array{
            std::pair{ U"\U000F0EFE", U"ID" },     std::pair{ U"\U000F0CB8", U"Title" },
            std::pair{ U"\U000F04BA", U"Sort" },   std::pair{ U"\U000F0803", U"Artist" },
            std::pair{ U"\U000F0770", U"Genre" },  std::pair{ U"\U000F0E2A", U"Music" },
            std::pair{ U"\U000F021F", U"Jacket" }, std::pair{ U"\U000F0337", U"URL" },
            std::pair{ U"\U000F05B7", U"Offset" }, std::pair{ U"\U000F07DA", U"BPM" },
        };

        [[nodiscard]]
        String PathText(const FilePath& path) {
            return path.isEmpty() ? U"" : FileSystem::RelativePath(path);
        }

        [[nodiscard]]
        String Ellipsize(const Font& font, const StringView text, const double font_size, const double max_width) {
            if (text.isEmpty() || max_width <= 0.0) {
                return U"";
            }
            if (font(text).region(font_size).w <= max_width) {
                return String{ text };
            }

            constexpr StringView Ellipsis = U"...";
            if (font(Ellipsis).region(font_size).w > max_width) {
                return U"";
            }

            std::size_t first = 0;
            std::size_t last = text.size();
            while (first < last) {
                const std::size_t middle = first + (last - first + 1) / 2;
                const String candidate = String{ text.substr(0, middle) } + Ellipsis;
                if (font(candidate).region(font_size).w <= max_width) {
                    first = middle;
                } else {
                    last = middle - 1;
                }
            }
            return String{ text.substr(0, first) } + Ellipsis;
        }
    }

    void SideMenu::draw(ViewerSession& session, const Rect& area) const {
        area.draw(ColorF{ 0.15, 0.16, 0.20 });

        const auto& font = SimpleGUI::GetFont();
        const auto& metadata = session.metadata();
        if (!metadata) {
            font(U"No metadata").draw(TextSize, Vec2{ area.x + Margin, area.y + 20 }, Palette::Lightgray);
            return;
        }

        const std::array<String, MetadataItems.size()> values{
            metadata->id,
            metadata->title,
            metadata->title_sort,
            metadata->artist,
            metadata->genre,
            PathText(metadata->music),
            PathText(metadata->jacket),
            metadata->url,
            U"{:.3f} s"_fmt(metadata->music_offset_seconds),
            U"{}"_fmt(metadata->bpm),
        };

        const double item_top = area.y + 20.0;
        const double value_width = area.rightX() - (area.x + Margin + TextOffset) - Margin;
        for (const auto& [index, item] : IndexedRef(MetadataItems)) {
            const double y = item_top + static_cast<double>(index) * ItemHeight;
            const auto& [icon, label] = item;
            font(icon).draw(IconSize, Arg::leftCenter = Vec2{ area.x + Margin, y }, ColorF{ 0.7 });
            font(label).draw(LabelSize, Arg::leftCenter = Vec2{ area.x + Margin + LabelOffset, y }, ColorF{ 0.7 });
            font(Ellipsize(font, values[index], TextSize, value_width))
                .draw(TextSize, Arg::leftCenter = Vec2{ area.x + Margin + TextOffset, y }, ColorF{ 0.95 });
        }

        const double separator_y = item_top + MetadataItems.size() * ItemHeight;
        Line{ area.x + Margin, separator_y, area.rightX() - Margin, separator_y }.draw(ColorF{ 0.45 });

        const double heading_y = separator_y + 22.0;
        font(U"\U000F041F").draw(IconSize, Arg::leftCenter = Vec2{ area.x + Margin, heading_y }, ColorF{ 0.7 });
        font(U"Difficulties")
            .draw(LabelSize, Arg::leftCenter = Vec2{ area.x + Margin + LabelOffset, heading_y }, ColorF{ 0.7 });

        for (const auto& [position, difficulty] : IndexedRef(metadata->difficulties)) {
            const RectF rect{
                area.x + Margin,
                heading_y + 14.0 + static_cast<double>(position) * DifficultyHeight,
                area.w - Margin * 2,
                DifficultyHeight,
            };
            const bool enabled = !session.isLoading() && !difficulty.src.isEmpty();
            const bool selected = session.selectedDifficultyPosition() == position;
            const ColorF color = enabled ? ColorF{ 0.95 } : ColorF{ 0.5 };

            if (enabled && rect.mouseOver()) {
                Cursor::RequestStyle(CursorStyle::Hand);
                rect.rounded(2).draw(ColorF{ 0.24 });
            }
            if (selected) {
                rect.rounded(2).draw(ColorF{ 0.28, 0.32, 0.42 });
            }
            if (enabled && rect.leftClicked() && !selected) {
                (void)session.selectDifficulty(position);
            }

            const Vec2 left_center = rect.leftCenter();
            font(selected ? U"\U000F043E" : U"\U000F043D").draw(LabelSize, Arg::leftCenter = left_center, color);
            font(U"{}"_fmt(difficulty.index))
                .draw(LabelSize, Arg::leftCenter = left_center.movedBy(LabelOffset, 0), color);
            const String difficulty_text = U"{}  Lv. {}"_fmt(difficulty.id, difficulty.level);
            const double difficulty_text_width = rect.rightX() - (left_center.x + 50.0) - Margin;
            font(Ellipsize(font, difficulty_text, TextSize, difficulty_text_width))
                .draw(TextSize, Arg::leftCenter = left_center.movedBy(50, 0), color);
        }

        if (session.jacket()) {
            session.jacket()
                ->fitted(Vec2{ 90, 90 })
                .draw(Arg::bottomLeft = Vec2{ area.x + Margin, area.bottomY() - Margin });
        }
    }
}
