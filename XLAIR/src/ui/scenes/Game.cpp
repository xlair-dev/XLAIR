#include "Game.hpp"

#include "app/consts/Scene.hpp"
#include "app/usecases/Assets.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/components/GameMusicPlate.hpp"
#include "ui/components/GameScoreBar.hpp"
#include "ui/primitives/SparkleShape.hpp"
#include "ui/primitives/Note.hpp"
#include "ui/addons/HomographyAddon.hpp"
#include "ui/theme/Palette.hpp"

namespace ui {
    Game::Game(const InitData& init) : IScene(init) {
        auto& data = getData();
        data.sheetRepository->loadDataAsync(
            data.userData.selected_index,
            data.userData.selected_difficulty
        );
    }
    Game::~Game() {
        auto& data = getData();
        data.sheetRepository->releaseData();
    }

    void Game::update() {
        auto& data = getData();
        m_scene_timer += Scene::DeltaTime();
        m_tile_offset = Math::Max(0.0, m_scene_timer - OffsetWait);
        m_pos_sample = AudioAsset(data.sheetRepository->AudioAssetName).posSample();

        if (not m_started and m_scene_timer >= 1.0) {
            if (data.sheetRepository->isDataReady()) {
                AudioAsset(data.sheetRepository->AudioAssetName).play();
            }
        }
    }

    void Game::draw() const {
        auto& data = getData();

        drawField();

        components::DrawUserNameplate(getData().userData, Point{ 59, 72 });
        components::DrawGameMusicPlate(Point{ 1480, 72 }, data.sheetRepository->getMetadata(data.userData.selected_index), data.sheetRepository->getJacket(data.userData.selected_index).value(), data.userData.selected_difficulty, m_tile_offset, 1);
        components::DrawGameScoreBar(Point{ app::consts::SceneWidth / 2 - 434, 56 }, data.score);
    }

    void Game::drawField() const {
        drawMainField();
        drawSideField();
    }

    void Game::drawMainField() const {
        const auto& data = getData();
        constexpr Quad field{
            Vec2{ 850, 0 },
            Vec2{ app::consts::SceneWidth - 850, 0},
            Vec2{ app::consts::SceneWidth - 225, app::consts::SceneHeight},
            Vec2{ 225, app::consts::SceneHeight },
        };

        {
            constexpr double EdgeMargin = 32.0;
            const auto field_size = m_rt_main_field.size();
            const auto [w, h] = field_size;
            const auto center_x = w / 2.0;
            const auto main_width = w - 2 * EdgeMargin;
            const ScopedRenderTarget2D target{ m_rt_main_field.clear(ColorF{ 1.0 }) };

            RectF{ field_size }.draw(ColorF{ U"#212830" });

            // lane dividers
            const auto lane_width = main_width / 4.0;
            Line{ center_x - lane_width, 0.0, center_x - lane_width, h }.draw(2.5, ColorF{ U"#FDFDFD" });
            Line{ center_x, 0.0, center_x, h }.draw(2.5, ColorF{ U"#FDFDFD" });
            Line{ center_x + lane_width, 0.0, center_x + lane_width, h }.draw(2.5, ColorF{ U"#FDFDFD" });

            // combo display
            FontAsset(app::assets::font::UiComboNumber)(U"{}"_fmt(data.score.combo)).drawAt(center_x, h - 750, theme::Palette::White);
            FontAsset(app::assets::font::UiSubText)(U"COMBO").drawAt(center_x, h - 600, theme::Palette::White);


            // judge line
            const double judge_y = h - JudgeLineY;
            constexpr ColorF JudgeColor = ColorF{ U"#79C8FF" };

            primitives::Sparkle(Vec2{ EdgeMargin - 2.0, judge_y }, 200.0, 300.0, 0.0, 16).asPolygon().drawFrame(1.5, JudgeColor);
            primitives::Sparkle(Vec2{ main_width + EdgeMargin + 2.0, judge_y }, 200.0, 300.0, 0.0, 16).asPolygon().drawFrame(1.5, JudgeColor);

            RectF{ Arg::leftCenter(0, judge_y), w, 5.0 }
                .drawShadow(Vec2{ 0.0, 10.0 }, 30.0, 0.0, JudgeColor.withA(0.15))
                .draw(JudgeColor); 
            // notes
            drawMainNotes();

            // Edge decorations
            RectF{ 0, 0, EdgeMargin, h }.draw(ColorF{ U"#B4E6FF" });
            RectF{ w - EdgeMargin, 0, EdgeMargin, h }.draw(ColorF{ U"#B4E6FF" });

            // blur
            RectF{ 0, 0, w, h / 1.75 }.draw(Arg::top = ColorF{ 1.0, 0.9 }, Arg::bottom = ColorF{ 1.0, 0.0 });
        }

        Graphics2D::Flush();
        m_rt_main_field.resolve();

        addons::HomographyAddon::Draw(field, m_rt_main_field);

        // debug draw
        RectF{ 1400, 0, 1000 / 4.0, 1000 }(m_rt_main_field).draw();
    }

    void Game::drawSideField() const {
    }

    void Game::drawMainNotes() const {
        auto& data = getData().sheetRepository->getData();
        if (not data.valid) {
            return;
        }
        constexpr double EdgeMargin = 32.0;
        const auto field_size = m_rt_main_field.size();
        const auto [w, h] = field_size;
        const auto center_x = w / 2.0;
        const auto main_width = w - 2 * EdgeMargin;
        const double lane_width = main_width / 16.0;

        //primitives::DrawHoldNoteHead(Vec2{ EdgeMargin, h - 100 }, lane_width * 4);
        for (const auto& tap : data.notes.tap) {
            const double x = EdgeMargin + lane_width * tap.start_lane;
            const double y = calculateNoteY(tap.sample);
            const double width = lane_width * tap.width;
            primitives::DrawTapNote(Vec2{ x, y }, width);
        }
    }

    double Game::calculateNoteY(int64 sample) const {
        return JudgeLineY - static_cast<double>(sample - m_pos_sample) * 0.02;
    }

    void Game::RegisterAssets() {
    }
}
