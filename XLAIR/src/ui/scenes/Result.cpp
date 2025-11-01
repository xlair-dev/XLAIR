#include "Result.hpp"
#include "app/consts/Scene.hpp"
#include "app/usecases/Assets.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/components/GameMusicPlate.hpp"
#include "ui/components/GameScoreBar.hpp"
#include "ui/components/SliderMappingGuide.hpp"
#include "core/features/ControllerManager.hpp"

namespace ui {

    Result::Result(const InitData& init) : IScene(init) {
    }

    Result::~Result() {
    }

    void Result::update() {
        m_scene_timer += Scene::DeltaTime();
        m_tile_offset = Math::Max(0.0, m_scene_timer - OffsetWait);

        if (m_scene_timer > 2.0) {
            bool any_touch = false;
            for (size_t i = 0; i < 32; ++i) {
                any_touch |= core::features::ControllerManager::Slider(i).frameCount() > 0;
            }
            if (any_touch or KeyEnter.down()) {
                changeScene(app::types::SceneState::MusicSelect, 0.5s);
            }
        }
    }

    void Result::draw() const {
        auto& data = getData();

        TextureAsset(app::assets::texture::ResultBackground).draw();

        components::DrawUserNameplate(getData().userData, Point{ 59, 72 });
        components::DrawGameMusicPlate(Point{ 1480, 72 }, data.sheetRepository->getMetadata(data.userData.selected_index), data.sheetRepository->getJacket(data.userData.selected_index).value(), data.userData.selected_difficulty, m_tile_offset, 1);
        components::DrawGameScoreBar(Point{ app::consts::SceneWidth / 2 - 434, 56 }, data.score);

        const auto t1 = Easing::Cubic(Math::Clamp((m_scene_timer - 0.25) / 0.25, 0.0, 1.0));
        const auto t2 = Easing::Cubic(Math::Clamp((m_scene_timer - 0.5) / 0.25, 0.0, 1.0));
        const auto t3 = Easing::Cubic(Math::Clamp((m_scene_timer - 0.5) / 0.5, 0.0, 1.0));

        const auto background = RectF{ Arg::topCenter(app::consts::SceneWidth / 2.0, 200), 1400, 760 };
        background.draw(ColorF{ 1.0, 0.7 * t1 });
        RectF{ background.pos, 16, 760 }.draw(theme::Palette::Cyan.withA(t1));
        RectF{ background.pos.movedBy(24, 0), 8, 760}.draw(theme::Palette::Cyan.withA(t1));
        RectF{ background.pos.movedBy(40, 0), 4, 760}.draw(theme::Palette::Cyan.withA(t1));
        RectF{ Arg::topRight(background.rightX(), background.y), 16, 760}.draw(theme::Palette::Cyan.withA(t1));
        RectF{ Arg::topRight(background.rightX() - 24, background.y), 8, 760}.draw(theme::Palette::Cyan.withA(t1));
        RectF{ Arg::topRight(background.rightX() - 40, background.y), 4, 760}.draw(theme::Palette::Cyan.withA(t1));

        {
            ScopedViewport2D viewport{ background.asRect() };
            FontAsset(app::assets::font::UiSubText)(U"SCORE").draw(40, 137, 42, ColorF{ 0.0, t1 });
            const String score_str = U"{}"_fmt(static_cast<int32>(data.score.score_value * t3));
            const size_t score_len = score_str.length();
            const auto r = FontAsset(app::assets::font::UiText)(String(8 - score_len, '0')).draw(140, 130, 50, theme::Palette::Gray.withA(0.6 * t2));
            FontAsset(app::assets::font::UiText)(score_str).draw(140, 130 + r.w, 50, theme::Palette::Cyan.withA(t2));

            TextureAsset(app::assets::texture::Perfect).draw(86, 397, ColorF{ 1.0, t1 });
            FontAsset(app::assets::font::UiText)(U"{}"_fmt(data.score.perfect_count)).draw(48, Arg::topRight(500, 385), ColorF{ 0.8, t2 });

            TextureAsset(app::assets::texture::Great).draw(86, 464, ColorF{ 1.0, t1 });
            FontAsset(app::assets::font::UiText)(U"{}"_fmt(data.score.great_count)).draw(48, Arg::topRight(500, 452), ColorF{ 0.8, t2 });

            TextureAsset(app::assets::texture::Good).draw(86, 530, ColorF{ 1.0, t1 });
            FontAsset(app::assets::font::UiText)(U"{}"_fmt(data.score.good_count)).draw(48, Arg::topRight(500, 518), ColorF{ 0.8, t2 });

            TextureAsset(app::assets::texture::Miss).draw(86, 598, ColorF{ 1.0, t1 });
            FontAsset(app::assets::font::UiText)(U"{}"_fmt(data.score.miss_count)).draw(48, Arg::topRight(500, 586), ColorF{ 0.8, t2 });

        }

        // controller
        Array<components::Mapping> mapping{
            {
                components::Mapping::Region{ 0, 16 },
                U"次へ",
                theme::Palette::Pink,
            },
        };
        components::DrawSliderMappingGuide(mapping);
    }

}
