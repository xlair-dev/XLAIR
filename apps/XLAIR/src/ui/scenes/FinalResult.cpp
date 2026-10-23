#include "FinalResult.hpp"

#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/AudioBus.hpp"
#include "ui/audio/SoundEffect.hpp"
#include "ui/components/ScrollingText.hpp"
#include "ui/input/SliderInput.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/presentation/DifficultyText.hpp"
#include "ui/presentation/ScoringText.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::scenes {
    namespace {
        constexpr std::size_t VisibleRecords = 5;

        [[nodiscard]]
        String SheetLabel(const Array<sheets::Metadata>& catalog, const StringView sheet_id) {
            for (const auto& music : catalog) {
                for (const auto& difficulty : music.difficulties) {
                    if (difficulty.id == sheet_id) {
                        return U"{} / {} Lv.{}"_fmt(
                            music.title,
                            presentation::DifficultyLabel(difficulty.index),
                            difficulty.level
                        );
                    }
                }
            }
            return String{ sheet_id };
        }
    }

    FinalResult::FinalResult(const InitData& init) : SceneBase{ init } {
        if (getData().music_select_context) {
            getData().music_select_context->pause();
        }
        m_scroll_mappings = {
            { .region = { .start = 0, .width = 8 },
              .label = localization::GetText(localization::TextId::FinalResultScrollUp),
              .color = theme::Palette::Pink },
            { .region = { .start = 8, .width = 8 },
              .label = localization::GetText(localization::TextId::FinalResultScrollDown),
              .color = theme::Palette::Pink },
        };
        AudioAsset{ assets::bgm::FinalResult }.play(audio::Music);
    }

    FinalResult::~FinalResult() {
        AudioAsset{ assets::bgm::FinalResult }.stop();
    }

    void FinalResult::update() {
        m_elapsed += Scene::DeltaTime();
        const auto& records = getData().application->playSession().playedRecords();
        const auto* controller = getData().application->controller();
        const bool up = KeyUp.down() || input::TouchRegionDown(controller, 0, 8);
        const bool down = KeyDown.down() || input::TouchRegionDown(controller, 8, 8);
        if (up && m_first_record > 0) {
            --m_first_record;
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
        } else if (down && m_first_record + VisibleRecords < records.size()) {
            ++m_first_record;
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
        }
    }

    void FinalResult::updateFadeIn(const double) {
        m_elapsed += Scene::DeltaTime();
    }

    void FinalResult::draw() const {
        Scene::Rect().draw(theme::Palette::White);
        const auto& application = *getData().application;
        const auto& session = application.playSession();
        const auto& records = session.playedRecords();
        FontAsset{ assets::font::Display }(U"FINAL RESULT")
            .drawAt(48, Vec2{ DesignSize.x / 2.0, 150 }, theme::Palette::Gray);
        FontAsset{ assets::font::Text }(U"TRACKS {} / {}"_fmt(records.size(), session.maxPlays()))
            .drawAt(26, Vec2{ DesignSize.x / 2.0, 215 }, theme::Palette::Gray);

        for (std::size_t row = 0; row < VisibleRecords && m_first_record + row < records.size(); ++row) {
            const std::size_t index = m_first_record + row;
            const auto& record = records[index].record;
            const int32 y = 290 + static_cast<int32>(row) * 128;
            FontAsset{ assets::font::Display }(index + 1).draw(28, Vec2{ 170, y + 8 }, theme::Palette::Cyan);
            components::DrawScrollingText(
                FontAsset{ assets::font::Text },
                SheetLabel(application.musicCatalog(), record.sheet_id),
                32,
                Rect{ 270, y, 850, 48 },
                m_elapsed,
                theme::Palette::Gray
            );
            FontAsset{ assets::font::Display }(U"{:07}"_fmt(record.score))
                .draw(42, Arg::topRight(1740, y), theme::Palette::Cyan);
            const StringView clear_label = record.clear_type == api::ClearType::Failed
                                               ? U"FAILED"
                                               : presentation::ClearStatusLabel(record.clear_type);
            FontAsset{ assets::font::Text }(clear_label).draw(22, Arg::topRight(1740, y + 56), theme::Palette::Gray);
        }

        FontAsset{ assets::font::Text }(localization::GetText(localization::TextId::FinalResultSavingUnavailable))
            .drawAt(24, Vec2{ DesignSize.x / 2.0, 960 }, theme::Palette::Gray);
        if (records.size() > VisibleRecords) {
            components::DrawSliderMappingGuide(m_scroll_mappings, RectF{ 210, 1010, 1500, 70 });
        }
    }

    void FinalResult::drawFadeIn(const double t) const {
        draw();
        Scene::Rect().draw(ColorF{ 0.0, 0.0, 0.0, 1.0 - Clamp(t, 0.0, 1.0) });
    }
}
