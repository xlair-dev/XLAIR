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
        constexpr double InputWaitSeconds = 2.0;

        [[nodiscard]]
        localization::TextId SaveStatus(const app::flows::FinalResult& flow) {
            using State = app::flows::FinalResult::State;
            using TextId = localization::TextId;
            switch (flow.state()) {
                case State::Idle:
                case State::SavingRecords:
                    return TextId::FinalResultSavingRecords;
                case State::SavingOptions:
                    return TextId::FinalResultSavingOptions;
                case State::Saved:
                    return TextId::FinalResultSaved;
                case State::Failed:
                    return flow.canRetry() ? TextId::FinalResultSaveFailed : TextId::FinalResultSaveUnconfirmed;
            }
            return TextId::FinalResultSaveUnconfirmed;
        }

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
        m_save_flow.start(getData().application->apiClient(), getData().application->playSession());
        AudioAsset{ assets::bgm::FinalResult }.play(audio::Music);
    }

    FinalResult::~FinalResult() {
        AudioAsset{ assets::bgm::FinalResult }.stop();
    }

    void FinalResult::update() {
        m_elapsed += Scene::DeltaTime();
        updateSave();
        const auto& records = getData().application->playSession().playedRecords();
        const auto* controller = getData().application->controller();
        const bool confirm = KeyEnter.down() || input::TouchRegionDown(controller, 0, 16);
        if (m_elapsed >= InputWaitSeconds && confirm) {
            if (m_save_flow.state() == app::flows::FinalResult::State::Saved) {
                AudioAsset{ assets::bgm::FinalResult }.stop(Duration{ 0.25 });
                audio::PlaySoundEffect(audio::SoundEffect::Confirm);
                changeScene(SceneState::Title, 500, CrossFade::No);
                return;
            }
            if (m_save_flow.canRetry()) {
                m_save_flow.retry();
                return;
            }
        }
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
        updateSave();
    }

    void FinalResult::updateSave() {
        const auto previous = m_save_flow.state();
        m_save_flow.update();
        if (m_save_flow.state() == app::flows::FinalResult::State::Failed && m_save_flow.state() != previous) {
            Logger << U"[FinalResult] Save failed: " + m_save_flow.error()->message;
        }
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
            const auto& played = records[index];
            const auto& record = played.record;
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
            const StringView clear_label = played.autoplay ? U"AUTO"
                                           : record.clear_type == api::ClearType::Failed
                                               ? U"FAILED"
                                               : presentation::ClearStatusLabel(record.clear_type);
            FontAsset{ assets::font::Text }(clear_label).draw(22, Arg::topRight(1740, y + 56), theme::Palette::Gray);
        }

        FontAsset{ assets::font::Text }(localization::GetText(SaveStatus(m_save_flow)))
            .drawAt(24, Vec2{ DesignSize.x / 2.0, 940 }, theme::Palette::Gray);
        if (const auto& error = m_save_flow.error()) {
            components::DrawScrollingText(
                FontAsset{ assets::font::Text },
                error->message,
                20,
                Rect{ 210, 972, 1500, 30 },
                m_elapsed,
                theme::Palette::Gray
            );
        }
        if (m_elapsed >= InputWaitSeconds &&
            (m_save_flow.state() == app::flows::FinalResult::State::Saved || m_save_flow.canRetry())) {
            const auto action = m_save_flow.canRetry() ? localization::TextId::FinalResultRetry
                                                       : localization::TextId::FinalResultReturnToTitle;
            components::DrawSliderMappingGuide(
                { { .region = { .start = 0, .width = 16 },
                    .label = localization::GetText(action),
                    .color = theme::Palette::Pink } },
                RectF{ 210, 1010, 1500, 70 }
            );
        } else if (records.size() > VisibleRecords) {
            components::DrawSliderMappingGuide(m_scroll_mappings, RectF{ 210, 1010, 1500, 70 });
        }
    }

    void FinalResult::drawFadeIn(const double t) const {
        draw();
        Scene::Rect().draw(ColorF{ 0.0, 0.0, 0.0, 1.0 - Clamp(t, 0.0, 1.0) });
    }

    void FinalResult::drawFadeOut(const double t) const {
        draw();
        Scene::Rect().draw(ColorF{ 0.0, 0.0, 0.0, Clamp(t, 0.0, 1.0) });
    }
}
