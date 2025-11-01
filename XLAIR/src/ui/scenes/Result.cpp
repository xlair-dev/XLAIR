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
        AudioAsset(app::assets::sounds::bgm::FinalResult).play();

        if (not m_update_record) {
            auto& data = getData();
            const auto& sheet_id = data.sheetRepository->getMetadata(data.userData.selected_index).difficulties[data.userData.selected_difficulty].id;
            core::types::Record record{};
            record.sheet_id = sheet_id;
            record.score = data.score.score_value;
            record.clear_type = core::types::ClearType::failed;
            if (data.score.clear_guage >= 0.7) {
                record.clear_type = core::types::ClearType::clear;
            }
            if (data.score.miss_count == 0) {
                record.clear_type = core::types::ClearType::fullcombo;

                if (data.score.good_count == 0 and data.score.great_count == 0) {
                    record.clear_type = core::types::ClearType::perfect;
                }
            }
            data.records[sheet_id] = record;
            data.played_records.push_back(record);
            m_update_record = true;

            // rating
            bool found = false;
            // レーティングは上位 3 曲の単曲レートの平均値を切り下げた値とする。
            Array<int32> song_ratings;
            for (const auto& [id, rec] : data.records) {
                // rec.sheet_id のレベルを探す
                double level = 0.0;
                const auto size = data.sheetRepository->size();
                for (size_t i = 0; i < size; ++i) {
                    const auto& metadata = data.sheetRepository->getMetadata(i);
                    for (const auto& diff : metadata.difficulties) {
                        if (diff.id == rec.sheet_id) {
                            level = diff.level;
                            found = true;
                            break;
                        }
                    }
                    if (found) {
                        break;
                    }
                }
                if (found) {
                    // rating 計算
                    //各譜面の単曲レートは以下のように計算される。単曲レートは整数である。以下に従って計算した後、切り下げを行う。
                    //    ベースレート = 楽曲の Level(0.1 刻みで表現される) * 100
                    //    スコア補正値 = スコアに応じて以下のように決定される。間の値の場合は線形補間する。
                    //    1, 090, 000 点以上(これ以上の値では線形補間しない) : +200
                    //    1, 050, 000 点以上 : +150
                    //    1, 000, 000 点以上 : +100
                    //    950, 000 点以上 : +50
                    //    900, 000 点以上 : +0
                    //    850, 000 点以上 : -50
                    //    800, 000 点以上 : -100
                    //    750, 000 点以上 : -150
                    //    700, 000 点以上(これ以下の値では線形補間しない) : -200
                    //    単曲レート = ベースレート + スコア補正値
                    const int32 base_rate = static_cast<int32>(level * 100.0);
                    int32 score_correction = 0;
                    const int32 score = static_cast<int32>(rec.score);
                    if (score >= 1090000) {
                        score_correction = 200;
                    } else if (score >= 1050000) {
                        score_correction = 150 + (score - 1050000) * (200 - 150) / (1090000 - 1050000);
                    } else if (score >= 1000000) {
                        score_correction = 100 + (score - 1000000) * (150 - 100) / (1050000 - 1000000);
                    } else if (score >= 950000) {
                        score_correction = 50 + (score - 950000) * (100 - 50) / (1000000 - 950000);
                    } else if (score >= 900000) {
                        score_correction = 0 + (score - 900000) * (50 - 0) / (950000 - 900000);
                    } else if (score >= 850000) {
                        score_correction = -50 + (score - 850000) * (0 + 50) / (900000 - 850000);
                    } else if (score >= 800000) {
                        score_correction = -100 + (score - 800000) * (-50 + 100) / (850000 - 800000);
                    } else if (score >= 750000) {
                        score_correction = -150 + (score - 750000) * (-100 + 150) / (800000 - 750000);
                    } else {
                        score_correction = -200;
                    }
                    const auto song_rating = base_rate + score_correction;
                    song_ratings.push_back(song_rating);
                }
            }
            song_ratings.sort_by(std::greater<>());
            int32 total_rating = 0;
            for (size_t i = 0; i < Min(song_ratings.size(), size_t{ 3 }); ++i) {
                total_rating += song_ratings[i];
            }
            m_rating = total_rating / Max(song_ratings.size(), size_t{ 3 });
        }

        if (m_scene_timer > 2.0) {
            bool any_touch = false;
            for (size_t i = 0; i < 32; ++i) {
                any_touch |= core::features::ControllerManager::Slider(i).frameCount() > 0;
            }
            if (any_touch or KeyEnter.down()) {
                getData().userData.rating = m_rating; // 更新
                getData().playable--;

                AudioAsset(app::assets::sounds::bgm::FinalResult).stop();
                AudioAsset(app::assets::sounds::se::Finish).playOneShot();
                if (getData().playable == 0) {
                    changeScene(app::types::SceneState::FinalResult, 0.5s);
                } else {
                    changeScene(app::types::SceneState::MusicSelect, 0.5s);
                }
            }
        }
    }

    void Result::draw() const {
        auto& data = getData();

        TextureAsset(app::assets::texture::ResultBackground).draw();

        components::DrawUserNameplate(getData().userData, Point{ 59, 72 });
        components::DrawGameMusicPlate(Point{ 1480, 72 }, data.sheetRepository->getMetadata(data.userData.selected_index), data.sheetRepository->getJacket(data.userData.selected_index).value(), data.userData.selected_difficulty, m_tile_offset, data.max_playable, data.playable);
        components::DrawGameScoreBar(Point{ app::consts::SceneWidth / 2 - 434, 56 }, data.score);

        const auto t1 = Easing::Cubic(Math::Clamp((m_scene_timer - 0.25) / 0.25, 0.0, 1.0));
        const auto t2 = Easing::Cubic(Math::Clamp((m_scene_timer - 0.5) / 0.25, 0.0, 1.0));
        const auto t3 = Easing::Cubic(Math::Clamp((m_scene_timer - 0.5) / 0.5, 0.0, 1.0));
        const auto t4 = Easing::Cubic(Math::Clamp((m_scene_timer - 1.0) / 0.25, 0.0, 1.0));

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
            const auto r = FontAsset(app::assets::font::UiText)(String(7 - score_len, '0')).draw(140, 130, 50, theme::Palette::Gray.withA(0.6 * t2));
            FontAsset(app::assets::font::UiText)(score_str).draw(140, 130 + r.w, 50, theme::Palette::Cyan.withA(t2));

            TextureAsset(app::assets::texture::Perfect).draw(86, 397, ColorF{ 1.0, t1 });
            FontAsset(app::assets::font::UiText)(U"{}"_fmt(data.score.perfect_count)).draw(48, Arg::topRight(500, 385), ColorF{ 0.8, t2 });

            TextureAsset(app::assets::texture::Great).draw(86, 464, ColorF{ 1.0, t1 });
            FontAsset(app::assets::font::UiText)(U"{}"_fmt(data.score.great_count)).draw(48, Arg::topRight(500, 452), ColorF{ 0.8, t2 });

            TextureAsset(app::assets::texture::Good).draw(86, 530, ColorF{ 1.0, t1 });
            FontAsset(app::assets::font::UiText)(U"{}"_fmt(data.score.good_count)).draw(48, Arg::topRight(500, 518), ColorF{ 0.8, t2 });

            TextureAsset(app::assets::texture::Miss).draw(86, 598, ColorF{ 1.0, t1 });
            FontAsset(app::assets::font::UiText)(U"{}"_fmt(data.score.miss_count)).draw(48, Arg::topRight(500, 586), ColorF{ 0.8, t2 });

            FontAsset(app::assets::font::UiSubText)(U"COMBO").draw(40, Arg::center(700, 410), ColorF{ 0.0, t1 });
            FontAsset(app::assets::font::UiText)(U"{}"_fmt(data.score.max_combo)).draw(80, Arg::center(700, 500), theme::Palette::Pink.withA(t2));

            Circle{ 1100, 500, 70 }.drawFrame(5, 0, theme::Palette::Purple.withA(0.7 * t1));
            FontAsset(app::assets::font::UiSubText)(U"RATE").draw(40, Arg::center(1100, 410), ColorF{ 0.0, t1 });
            FontAsset(app::assets::font::UiSubText)(U"{}"_fmt(m_rating)).draw(70, Arg::center(1100, 500), ColorF{0.0, t1});

            // grade
            TextureAsset(app::assets::texture::GradeBack).draw(Arg::center(1050, 150), ColorF{ 1.0, t1 });

            const auto score_value = data.score.score_value;
            if (score_value >= 1'050'000) {
                // SSS
                TextureAsset(app::assets::texture::GradeS).draw(Arg::center(1050 - 60, 150), ColorF{ 1.0, t4 });
                TextureAsset(app::assets::texture::GradeS).draw(Arg::center(1050, 150), ColorF{ 1.0, t4 });
                TextureAsset(app::assets::texture::GradeS).draw(Arg::center(1050 + 60, 150), ColorF{ 1.0, t4 });
            } else if (score_value >= 1'000'000) {
                // SS
                TextureAsset(app::assets::texture::GradeS).draw(Arg::center(1050 - 30, 150), ColorF{ 1.0, t4 });
                TextureAsset(app::assets::texture::GradeS).draw(Arg::center(1050 + 30, 150), ColorF{ 1.0, t4 });
            } else if (score_value >= 950'000) {
                // S
                TextureAsset(app::assets::texture::GradeS).draw(Arg::center(1050, 150), ColorF{ 1.0, t4 });
            } else if (score_value >= 900'000) {
                // AAA
                TextureAsset(app::assets::texture::GradeA).draw(Arg::center(1050 - 60, 150), ColorF{ 1.0, t4 });
                TextureAsset(app::assets::texture::GradeA).draw(Arg::center(1050, 150), ColorF{ 1.0, t4 });
                TextureAsset(app::assets::texture::GradeA).draw(Arg::center(1050 + 60, 150), ColorF{ 1.0, t4 });
            } else if (score_value >= 850'000) {
                // AA
                TextureAsset(app::assets::texture::GradeA).draw(Arg::center(1050 - 30, 150), ColorF{ 1.0, t4 });
                TextureAsset(app::assets::texture::GradeA).draw(Arg::center(1050 + 30, 150), ColorF{ 1.0, t4 });
            } else if (score_value >= 800'000) {
                // A
                TextureAsset(app::assets::texture::GradeA).draw(Arg::center(1050, 150), ColorF{ 1.0, t4 });
            } else if (score_value >= 750'000) {
                // BBB
                TextureAsset(app::assets::texture::GradeB).draw(Arg::center(1050 - 60, 150), ColorF{ 1.0, t4 });
                TextureAsset(app::assets::texture::GradeB).draw(Arg::center(1050, 150), ColorF{ 1.0, t4 });
                TextureAsset(app::assets::texture::GradeB).draw(Arg::center(1050 + 60, 150), ColorF{ 1.0, t4 });
            } else if (score_value >= 700'000) {
                // BB
                TextureAsset(app::assets::texture::GradeB).draw(Arg::center(1050 - 30, 150), ColorF{ 1.0, t4 });
                TextureAsset(app::assets::texture::GradeB).draw(Arg::center(1050 + 30, 150), ColorF{ 1.0, t4 });
            } else {
                // B
                TextureAsset(app::assets::texture::GradeB).draw(Arg::center(1050, 150), ColorF{ 1.0, t4 });
            }

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
