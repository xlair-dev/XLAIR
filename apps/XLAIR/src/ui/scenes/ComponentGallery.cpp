#include "ComponentGallery.hpp"

#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/components/GameMusicPlate.hpp"
#include "ui/components/MenuTimerPlate.hpp"
#include "ui/components/SliderMappingGuide.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/primitives/Arrow.hpp"
#include "ui/primitives/Sparkle.hpp"
#include "ui/theme/DifficultyTheme.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::scenes {
    namespace {
        Texture MakeJacket() {
            Image image{ 512, 512, Color{ 28, 36, 58 } };
            Circle{ 256, 256, 190 }.overwrite(image, Color{ theme::Palette::Purple });
            Circle{ 256, 256, 135 }.overwrite(image, Color{ theme::Palette::Cyan });
            Circle{ 256, 256, 80 }.overwrite(image, Color{ theme::Palette::Pink });
            return Texture{ image, TextureDesc::Mipped };
        }

        constexpr std::array<StringView, 3> DifficultyLabels{
            U"Basic",
            U"Advanced",
            U"Master",
        };

        constexpr std::array<uint32, 3> HighScores{
            0,
            975'000,
            1'050'000,
        };

        constexpr std::array<StringView, 3> Grades{
            U"",
            U"S",
            U"SSS",
        };

        constexpr std::array<StringView, 3> ClearStatuses{
            U"",
            U"CLEAR",
            U"FULL COMBO",
        };

        const Array<components::SliderMapping> SliderMappings{
            {
                .region = { .start = 0, .width = 3, .right_corner = false },
                .label = U"左",
                .color = theme::Palette::Pink,
            },
            {
                .region = { .start = 3, .width = 3, .left_corner = false },
                .label = U"右",
                .color = theme::Palette::Pink,
            },
            {
                .region = { .start = 6, .width = 4 },
                .label = U"選択",
                .color = theme::Palette::Cyan,
            },
            {
                .region = { .start = 10, .width = 2, .right_corner = false },
                .label = U"レベル -",
                .color = theme::Palette::Pink,
            },
            {
                .region = { .start = 12, .width = 2, .left_corner = false },
                .label = U"レベル +",
                .color = theme::Palette::Pink,
            },
            {
                .region = { .start = 14, .width = 2 },
                .label = U"設定",
                .color = theme::Palette::Purple,
            },
        };
    }

    ComponentGallery::ComponentGallery(const InitData& init) : SceneBase{ init }, m_jacket{ MakeJacket() } {}

    void ComponentGallery::update() {
        m_elapsed += Scene::DeltaTime();
        if (KeyEscape.down()) {
            changeScene(SceneState::Title, 0);
            return;
        }
    }

    void ComponentGallery::draw() const {
        Scene::Rect().draw(theme::Palette::White);

        FontAsset{ assets::font::Display }(U"COMPONENT GALLERY")
            .drawAt(42, Vec2{ DesignSize.x / 2.0, 44 }, theme::Palette::DimmedPurple);
        FontAsset{ assets::font::Label }(U"ESC: TITLE")
            .draw(18, Arg::topRight(Vec2{ DesignSize.x - 24.0, 24.0 }), theme::Palette::LightGray);

        drawGameHud();

        constexpr double CardScale = 0.68;
        constexpr SizeF CardSize = components::MusicCard::size() * CardScale;
        constexpr std::array<double, 4> CardCenters{ 240.0, 720.0, 1200.0, 1680.0 };
        for (std::size_t index = 0; index < DifficultyLabels.size(); ++index) {
            const components::MusicCardData data{
                .title = U"Very loooooooooooooong title",
                .artist = U"Artist",
                .difficulty = DifficultyLabels[index],
                .level = 3.0 + index * 4.5,
                .designer = U"XLAIR",
                .high_score = HighScores[index],
                .grade = Grades[index],
                .clear_status = ClearStatuses[index],
            };
            const RectF region{ Arg::center = Vec2{ CardCenters[index], 510 }, CardSize };
            region.drawShadow(Vec2{ 8, 16 }, 20, 0, ColorF{ 0, 0, 0, 0.18 });
            region(
                m_music_card.render(data, m_jacket, theme::GetDifficultyTheme(static_cast<uint32>(index)), m_elapsed)
            )
                .draw();
        }

        const components::SettingCardData setting_data{
            .title = U"判定タイミングの変更",
            .description =
                U"設定に関する説明がここに文章でたくさん入るよ。最低1行で最大3行を想定しているよ。このくらいが最大。",
            .value = U"1.00",
        };
        const RectF setting_region{
            Arg::center = Vec2{ CardCenters.back(), 510 },
            components::SettingCard::size() * CardScale,
        };
        setting_region.drawShadow(Vec2{ 8, 16 }, 20, 0, ColorF{ 0, 0, 0, 0.18 });
        setting_region(m_setting_card.render(setting_data)).draw();

        constexpr double PrimitiveY = 815;
        primitives::DrawArrow(Vec2{ 760, PrimitiveY }, primitives::ArrowDirection::Left, theme::Palette::Gray, 0.7);
        primitives::DrawArrow(Vec2{ 1160, PrimitiveY }, primitives::ArrowDirection::Right, theme::Palette::Gray, 0.7);
        primitives::Sparkle(Vec2{ 900, PrimitiveY }, 28, 45).draw(theme::Palette::Pink);
        primitives::Sparkle(Vec2{ 960, PrimitiveY }, 40, 40).draw(theme::Palette::Purple);
        primitives::Sparkle(Vec2{ 1020, PrimitiveY }, 28, 45).draw(theme::Palette::Cyan);

        FontAsset{ assets::font::Label }(U"Arrow / Sparkle")
            .drawAt(20, Vec2{ DesignSize.x / 2.0, PrimitiveY + 70 }, theme::Palette::LightGray);

        components::DrawUserNameplate(
            {
                .display_name = U"Player",
                .rating = 1250,
                .level = 7,
                .level_progress = 0.53,
            },
            Point{ 80, 760 }
        );
        components::DrawMenuTimerPlate(
            {
                .remaining_seconds = 58,
                .max_plays = 3,
                .remaining_plays = 2,
            },
            Point{ 1510, 760 }
        );

        components::DrawSliderMappingGuide(SliderMappings, RectF{ 210, 980, 1500, 100 });
    }

    void ComponentGallery::drawGameHud() const {
        components::DrawGameMusicPlate(
            {
                .title = U"Very loooooooooooooong title",
                .jacket = m_jacket,
                .difficulty_index = 2,
                .level = 12.0,
                .max_plays = 3,
                .remaining_plays = 2,
            },
            Point{ 1480, 72 },
            m_elapsed
        );
        m_game_score_bar.draw({ .score = 765'432, .clear_gauge = 0.74 }, Point{ DesignSize.x / 2 - 434, 56 });
    }
}
