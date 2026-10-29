#include "NameEntryPanel.hpp"

#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::components {
    namespace {
        constexpr SizeF CharacterSize{ 172, 210 };
        constexpr Vec2 CharacterCenter{ DesignSize.x / 2.0, 655 };
    }

    NameEntryPanel::NameEntryPanel() : m_carousel{ CharacterSize, CharacterCenter, DesignSize.x } {}

    void NameEntryPanel::animateMove(const int32 direction) {
        m_carousel.animateMove(direction);
    }

    void NameEntryPanel::update(const double delta_seconds) {
        m_carousel.update(delta_seconds);
    }

    void NameEntryPanel::draw(const input::NameEntry& entry) const {
        constexpr RectF NameRegion{ 460, 275, 1000, 165 };
        NameRegion.drawShadow(Vec2{ 0, 6 }, 20, 0, ColorF{ 0.3, 0.08 });
        NameRegion.draw(Palette::White).drawFrame(1, 0, theme::Palette::Cyan);
        Triangle{ NameRegion.tl(), NameRegion.tl().movedBy(24, 0), NameRegion.tl().movedBy(0, 24) }.draw(
            theme::Palette::Pink
        );
        FontAsset{ assets::font::Text }(localization::GetText(localization::TextId::LoginNameLabel))
            .draw(22, NameRegion.pos.movedBy(30, 15), theme::Palette::BlueGray);

        const String& name = entry.name();
        const StringView text = name.isEmpty()
                                    ? StringView{ localization::GetText(localization::TextId::LoginNameEmpty) }
                                    : StringView{ name };
        const ColorF name_color = name.isEmpty() ? theme::Palette::LightGray : theme::Palette::BlueGray;
        const auto name_text = FontAsset{ assets::font::Text }(text);
        const double name_size = Min(60.0, 60.0 * 860.0 / Max(1.0, name_text.region(60).w));
        const auto name_bounds = name_text.drawAt(name_size, NameRegion.center().movedBy(0, 10), name_color);
        if (!name.isEmpty()) {
            Line{ name_bounds.rightCenter().movedBy(8, -25), name_bounds.rightCenter().movedBy(8, 25) }.draw(
                2,
                theme::Palette::Cyan
            );
        }
        FontAsset{ assets::font::Label }(U"{} / {}"_fmt(entry.length(), input::NameEntry::MaximumLength))
            .draw(20, Arg::bottomRight = NameRegion.br().movedBy(-22, -10), theme::Palette::Gray);

        constexpr double CategoryWidth = 170;
        const double category_left = (DesignSize.x - CategoryWidth * input::NameEntry::categoryCount()) / 2.0;
        for (std::size_t index = 0; index < input::NameEntry::categoryCount(); ++index) {
            const bool selected = index == entry.categoryIndex();
            const Vec2 center{ category_left + CategoryWidth * (index + 0.5), 488 };
            FontAsset{ assets::font::Label }(input::NameEntry::categoryLabel(index))
                .drawAt(27, center, selected ? theme::Palette::Cyan : theme::Palette::LightGray);
            if (selected) {
                Line{ center.movedBy(-50, 23), center.movedBy(50, 23) }
                    .draw(3, theme::Palette::Pink, theme::Palette::Cyan);
            }
        }

        const auto& characters = entry.characters();
        for (const auto& placement : m_carousel.layout(entry.selectedIndex(), characters.size())) {
            const double scale = placement.region.h / CharacterSize.y;
            const ColorF accent = placement.selected ? theme::Palette::Pink : theme::Palette::Cyan;
            const auto& region = placement.region;
            region.drawShadow(Vec2{ 0, 5 }, 16, 0, ColorF{ 0.3, 0.06 });
            region.draw(Palette::White).drawFrame(placement.selected ? 2 : 1, 0, accent);
            const char32 character = characters[placement.index];
            const String label = character == U' ' ? U"SPACE" : String{ character };
            FontAsset{ assets::font::Text }(label)
                .drawAt(character == U' ' ? 27 * scale : 94 * scale, region.center(), theme::Palette::BlueGray);
            if (placement.selected) {
                Line{ region.bl().movedBy(20, -15), region.br().movedBy(-20, -15) }
                    .draw(3, theme::Palette::Pink, theme::Palette::Cyan);
            }
        }
        m_carousel.drawArrows(entry.selectedIndex(), characters.size(), theme::Palette::Gray);
    }
}
