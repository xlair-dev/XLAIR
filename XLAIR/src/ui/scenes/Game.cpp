#include "Game.hpp"

#include "core/features/ControllerManager.hpp"
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
                m_samples = AudioAsset(data.sheetRepository->AudioAssetName).samples();
                AudioAsset(data.sheetRepository->AudioAssetName).play();
                m_started = true;
            }
        }

        if (m_started) {
            if (not AudioAsset(data.sheetRepository->AudioAssetName).isPlaying()) {
                changeScene(app::types::SceneState::Result);
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

            // touch effect
            drawTouchEffect();

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
        const auto main_width = w - 2 * EdgeMargin;
        const double lane_width = main_width / 16.0;

        //primitives::DrawHoldNoteHead(Vec2{ EdgeMargin, h - 100 }, lane_width * 4);
        for (const auto& hold : data.notes.hold) {
            for (size_t i = 1; i < hold.notes.size(); i++) {
                // 先にスライドを描画してあとから Head を描画する
                const auto& pre = hold.notes[i - 1];
                const auto& cur = hold.notes[i];

                const double cur_x = EdgeMargin + lane_width * cur.start_lane;
                const double cur_y = calculateNoteY(cur.sample);
                const double cur_w = lane_width * cur.width;

                const double pre_x = EdgeMargin + lane_width * pre.start_lane;
                const double pre_y = calculateNoteY(pre.sample);
                const double pre_w = lane_width * pre.width;

                if (pre.type == SheetsAnalyzer::HoldNoteType::Start or pre.type == SheetsAnalyzer::HoldNoteType::End) {
                    primitives::DrawHoldNoteHead(Vec2{ pre_x, pre_y }, pre_w);
                }

                primitives::DrawHoldBack(
                    Vec2{ pre_x, pre_y }, pre_w,
                    Vec2{ cur_x, cur_y }, cur_w
                );
            }
            if (not hold.notes.isEmpty() and hold.notes.back().type == SheetsAnalyzer::HoldNoteType::Start or hold.notes.back().type == SheetsAnalyzer::HoldNoteType::End) {
                const double x = EdgeMargin + lane_width * hold.notes.back().start_lane;
                const double y = calculateNoteY(hold.notes.back().sample);
                const double width = lane_width * hold.notes.back().width;
                primitives::DrawHoldNoteHead(Vec2{ x, y }, width);
            }
        }

        for (const auto& tap : data.notes.tap) {
            const double y = calculateNoteY(tap.sample);
            if (y < -50 or h + 50 < y) {
                continue;
            }

            const double x = EdgeMargin + lane_width * tap.start_lane;
            const double width = lane_width * tap.width;
            primitives::DrawTapNote(Vec2{ x, y }, width);
        }

        for (const auto& tap : data.notes.xtap) {
            const double y = calculateNoteY(tap.sample);
            if (y < -50 or h + 50 < y) {
                continue;
            }
            const double x = EdgeMargin + lane_width * tap.start_lane;
            const double width = lane_width * tap.width;
            primitives::DrawXTapNote(Vec2{ x, y }, width);
        }

        for (const auto& tap : data.notes.flick) {
            const double y = calculateNoteY(tap.sample);
            const double x = EdgeMargin + lane_width * tap.start_lane;
            if (y < -50 or h + 50 < y) {
                continue;
            }
            const double width = lane_width * tap.width;
            primitives::DrawFlickNote(Vec2{ x, y }, width);
        }


    }

    void Game::drawTouchEffect() const {
        using core::features::ControllerManager;
        constexpr double EdgeMargin = 32.0;
        const auto field_size = m_rt_main_field.size();
        const auto [w, h] = field_size;
        const auto main_width = w - 2 * EdgeMargin;
        const double lane_width = main_width / 16.0;

        for (uint8 i = 0; i < 16; ++i) {
            const bool pressed = ControllerManager::Slider(2 * i).pressed() or ControllerManager::Slider(2 * i + 1).pressed();
            if (pressed) {
                const double x = EdgeMargin + lane_width * i;
                RectF{ Arg::bottomLeft(x, h), lane_width, 250 }.draw(
                    Arg::top = ColorF{ 1.0, 0.0 },
                    Arg::bottom = ColorF{ 1.0, 0.7 }
                );
            }
        }
    }

    void Game::judgement() {
    }

    double Game::calculateNoteY(int64 sample) const {
        constexpr double h = 4000;
        return (4000 - JudgeLineY) - static_cast<double>(sample - m_pos_sample) * 0.08;
    }

    void Game::RegisterAssets() {
    }
}
