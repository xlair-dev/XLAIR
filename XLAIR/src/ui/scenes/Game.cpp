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
                m_sample_rate = AudioAsset(data.sheetRepository->AudioAssetName).sampleRate();
                AudioAsset(data.sheetRepository->AudioAssetName).play();
                m_started = true;
            }
        }

        if (m_started) {
            if (not AudioAsset(data.sheetRepository->AudioAssetName).isPlaying()) {
                changeScene(app::types::SceneState::Result);
            }
        }

        judgement();

        ClearPrint();
        Print << data.score.perfect_count;
        Print << data.score.great_count;
        Print << data.score.good_count;
        Print << data.score.miss_count;
        Print << data.score.combo;
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
            if (hold.notes.isEmpty()) {
                continue;
            }
            const double start_sample = hold.notes.front().sample;
            const double length = static_cast<double>(hold.notes.back().sample - hold.notes.front().sample);
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

                const double cur_t = (cur.sample - start_sample) / length;
                const double pre_t = (pre.sample - start_sample) / length;
                primitives::DrawHoldBack(
                    Vec2{ pre_x, pre_y }, pre_w, pre_t,
                    Vec2{ cur_x, cur_y }, cur_w, cur_t
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
            if (tap.done) {
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
            if (tap.done) {
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
            if (tap.done) {
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
        // TODO: ここらへんのロジックは core/features などに移すべきかも
        // TODO: 流石にオワリ・実装なのでマシなものにするべき
        using core::features::ControllerManager;
        auto& data = getData().sheetRepository->m_data;
        auto& score = getData().score;
        if (not data.valid) {
            return;
        }
        // 35ms, 65ms, 100ms
        const int64 perfect = static_cast<int64>(0.035 * m_sample_rate);
        const int64 great = static_cast<int64>(0.065 * m_sample_rate);
        const int64 good = static_cast<int64>(0.100 * m_sample_rate);

        Array<bool> has_judged(16, false);

        size_t tap_index = 0;
        size_t xtap_index = 0;
        size_t hold_index = 0;
        while (true) {
            int32 min_target = -1;
            int64 value = std::numeric_limits<int64>::max();
            for (; tap_index < data.notes.tap.size(); ++tap_index) {
                auto& tap = data.notes.tap[tap_index];
                if (tap.done) {
                    continue;
                }
                if (tap.passed) {
                    const double y = calculateNoteY(tap.sample);
                    if (y < -50) {
                        tap.done = true;
                    }
                    continue;
                }
                if (value > tap.sample) {
                    min_target = 0;
                    value = tap.sample;
                    break;
                }
            }

            for (; xtap_index < data.notes.xtap.size(); ++xtap_index) {
                auto& xtap = data.notes.xtap[xtap_index];
                if (xtap.done) {
                    continue;
                }
                if (xtap.passed) {
                    const double y = calculateNoteY(xtap.sample);
                    if (y < -50) {
                        xtap.done = true;
                    }
                    continue;
                }
                if (value > xtap.sample) {
                    min_target = 1;
                    value = xtap.sample;
                    break;
                }
            }

            for (; hold_index < data.notes.hold.size(); ++hold_index) {
                auto& hold = data.notes.hold[hold_index];
                // TODO:
            }

            if (min_target == -1) {
                break;
            }

            if (min_target == 0) {
                auto& tap = data.notes.tap[tap_index];
                const int64 dist = tap.sample - m_pos_sample; // TODO: add timing setting
                if (dist < -good) {
                    // miss
                    score.miss_count++;
                    score.combo = 0;
                    tap.passed = true;
                }

                bool pressed = false;
                for (size_t i = 0; i < tap.width; ++i) {
                    const auto index = tap.start_lane + i;
                    if (has_judged[index]) {
                        continue;
                    }
                    pressed |= ControllerManager::Slider(2 * index).down();
                    pressed |= ControllerManager::Slider(2 * index + 1).down();
                }

                if (pressed) {
                    const auto adist = Abs(dist);
                    if (adist <= good) {
                        score.combo++;
                        tap.done = true;
                        for (size_t i = 0; i < tap.width; ++i) {
                            has_judged[tap.start_lane + i] = true;
                        }
                    }

                    if (adist <= perfect) {
                        score.perfect_count++;
                    } else if (adist <= great) {
                        score.great_count++;
                    } else if (adist <= good) {
                        score.good_count++;
                    }
                }
                tap_index++;
            }

            if (min_target == 1) {
                auto& xtap = data.notes.xtap[xtap_index];
                const int64 dist = xtap.sample - m_pos_sample; // TODO: add timing setting
                if (dist < -good) {
                    // miss
                    score.miss_count++;
                    score.combo = 0;
                    xtap.passed = true;
                }

                bool pressed = false;
                for (size_t i = 0; i < xtap.width; ++i) {
                    const auto index = xtap.start_lane + i;
                    if (has_judged[index]) {
                        continue;
                    }
                    pressed |= ControllerManager::Slider(2 * index).down();
                    pressed |= ControllerManager::Slider(2 * index + 1).down();
                }

                if (pressed) {
                    const auto adist = Abs(dist);
                    if (adist <= good) {
                        score.combo++;
                        xtap.done = true;
                        score.perfect_count++;
                        for (size_t i = 0; i < xtap.width; ++i) {
                            has_judged[xtap.start_lane + i] = true;
                        }
                    }
                }
                xtap_index++;
            }
        }
    }

    double Game::calculateNoteY(int64 sample) const {
        constexpr double h = 4000;
        return (4000 - JudgeLineY) - static_cast<double>(sample - m_pos_sample) * 0.07;
    }

    void Game::RegisterAssets() {
    }
}
