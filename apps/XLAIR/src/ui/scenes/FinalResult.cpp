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
        constexpr components::SliderMapping::Region ScrollUpRegion{ .start = 0, .width = 5 };
        constexpr components::SliderMapping::Region ConfirmRegion{ .start = 5, .width = 6 };
        constexpr components::SliderMapping::Region ScrollDownRegion{ .start = 11, .width = 5 };

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
        const auto& application = *getData().application;
        for (const auto& played : application.playSession().playedResults()) {
            const StringView clear_label = played.autoplay ? U"AUTO"
                                           : played.clear_type == api::ClearType::Failed
                                               ? U"FAILED"
                                               : presentation::ClearStatusLabel(played.clear_type);
            m_rows.push_back(
                {
                    .sheet_label = SheetLabel(application.musicCatalog(), played.sheet_id),
                    .score_label = U"{:07}"_fmt(played.score),
                    .clear_label = String{ clear_label },
                }
            );
        }
        m_save_flow.start(getData().application->apiClient(), getData().application->playSession());
        AudioAsset{ assets::bgm::FinalResult }.play(audio::Music);
    }

    FinalResult::~FinalResult() {
        AudioAsset{ assets::bgm::FinalResult }.stop();
    }

    void FinalResult::update() {
        m_elapsed += Scene::DeltaTime();
        updateSave();
        const auto* controller = getData().application->controller();
        const bool scrollable = m_rows.size() > VisibleRecords;
        const bool confirm = KeyEnter.down() ||
                             (scrollable ? input::TouchRegionDown(controller, ConfirmRegion.start, ConfirmRegion.width)
                                         : input::TouchRegionDown(controller, 0, 16));
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
        const bool up = KeyUp.down() ||
                        (scrollable && input::TouchRegionDown(controller, ScrollUpRegion.start, ScrollUpRegion.width));
        const bool down =
            KeyDown.down() ||
            (scrollable && input::TouchRegionDown(controller, ScrollDownRegion.start, ScrollDownRegion.width));
        if (up && m_first_record > 0) {
            --m_first_record;
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
        } else if (down && m_first_record + VisibleRecords < m_rows.size()) {
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
        FontAsset{ assets::font::Display }(U"FINAL RESULT")
            .drawAt(48, Vec2{ DesignSize.x / 2.0, 150 }, theme::Palette::Gray);
        FontAsset{ assets::font::Text }(U"TRACKS {} / {}"_fmt(m_rows.size(), session.maxPlays()))
            .drawAt(26, Vec2{ DesignSize.x / 2.0, 215 }, theme::Palette::Gray);

        for (std::size_t row = 0; row < VisibleRecords && m_first_record + row < m_rows.size(); ++row) {
            const std::size_t index = m_first_record + row;
            const auto& record = m_rows[index];
            const int32 y = 290 + static_cast<int32>(row) * 128;
            FontAsset{ assets::font::Display }(index + 1).draw(28, Vec2{ 170, y + 8 }, theme::Palette::Cyan);
            components::DrawScrollingText(
                FontAsset{ assets::font::Text },
                record.sheet_label,
                32,
                Rect{ 270, y, 850, 48 },
                m_elapsed,
                theme::Palette::Gray
            );
            FontAsset{ assets::font::Display }(record.score_label)
                .draw(42, Arg::topRight(1740, y), theme::Palette::Cyan);
            FontAsset{ assets::font::Text }(record.clear_label)
                .draw(22, Arg::topRight(1740, y + 56), theme::Palette::Gray);
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
        Array<components::SliderMapping> mappings;
        const bool scrollable = m_rows.size() > VisibleRecords;
        if (scrollable) {
            mappings.push_back(
                { .region = ScrollUpRegion,
                  .label = localization::GetText(localization::TextId::FinalResultScrollUp),
                  .color = theme::Palette::Pink }
            );
        }
        if (m_elapsed >= InputWaitSeconds &&
            (m_save_flow.state() == app::flows::FinalResult::State::Saved || m_save_flow.canRetry())) {
            const auto action = m_save_flow.canRetry() ? localization::TextId::FinalResultRetry
                                                       : localization::TextId::FinalResultReturnToTitle;
            mappings.push_back(
                { .region = scrollable ? ConfirmRegion : components::SliderMapping::Region{ .start = 0, .width = 16 },
                  .label = localization::GetText(action),
                  .color = theme::Palette::Pink }
            );
        }
        if (scrollable) {
            mappings.push_back(
                { .region = ScrollDownRegion,
                  .label = localization::GetText(localization::TextId::FinalResultScrollDown),
                  .color = theme::Palette::Pink }
            );
        }
        if (!mappings.isEmpty()) {
            components::DrawSliderMappingGuide(mappings, RectF{ 210, 1010, 1500, 70 });
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
