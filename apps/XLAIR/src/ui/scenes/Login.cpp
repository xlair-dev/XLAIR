#include "Login.hpp"

#include "core/user/Level.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/AudioBus.hpp"
#include "ui/audio/SoundEffect.hpp"
#include "ui/components/MenuHeader.hpp"
#include "ui/components/MenuTimerPlate.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/theme/Palette.hpp"
#include "ui/transitions/ScreenWipe.hpp"

namespace xlair::ui::scenes {
    namespace {
        using TextId = localization::TextId;
        using SideButton = app::controller::SideButton;
        using LoginState = app::flows::Login::State;
        constexpr auto CancelButton = app::controller::MaintenanceButton::Button2;
        constexpr double CreditWaitSeconds = 30.0;
        constexpr std::array MenuKeys{ KeyLeft, KeyRight, KeySpace, KeyDown, KeyUp };
        constexpr double StepTransitionSeconds = transitions::CardTransitionMillisec / 1000.0;
        constexpr RectF SummaryRegion{ 460, 340, 1000, 370 };
        constexpr uint32 DeleteStart = 10;
        constexpr uint32 DeleteWidth = 3;
        constexpr uint32 DoneStart = 13;
        constexpr uint32 DoneWidth = 3;
        constexpr uint32 ReturnStart = 12;
        constexpr uint32 ReturnWidth = 4;

        const String& Text(const TextId id) {
            return localization::GetText(id);
        }

        void DrawPanel(const RectF& region, const ColorF& accent) {
            region.drawShadow(Vec2{ 0, 8 }, 24, 0, ColorF{ 0.3, 0.08 });
            region.draw(Palette::White).drawFrame(2, 0, accent);
            Triangle{ region.tl(), region.tl().movedBy(26, 0), region.tl().movedBy(0, 26) }.draw(theme::Palette::Pink);
        }

        void DrawSideLabel(const bool right) {
            components::DrawSliderMappingGuide(
                {
                    {
                        .region = { .start = 0, .width = 4, .left_corner = right, .right_corner = !right },
                        .label = right ? Text(TextId::LoginCategory) + U" ▶" : U"◀ " + Text(TextId::LoginCategory),
                        .color = theme::Palette::Cyan,
                    },
                },
                RectF{ right ? DesignSize.x - 300.0 : 0.0, 845, 300, 70 },
                4
            );
        }
    }

    Login::Login(const InitData& init)
        : SceneBase{ init }, m_visibility_carousel{ SizeF{ 530, 340 }, Vec2{ DesignSize.x / 2.0, 570 }, DesignSize.x },
          m_completion{ AudioAsset{ assets::sound::LoginComplete }.lengthSec() },
          m_credit_wait_timeout{ CreditWaitSeconds } {
        ClearPrint();
        AudioAsset{ assets::bgm::Login }.play(audio::Music);

        const auto& data = getData();
        if (!data.scanned_card) {
            m_local_error = Text(TextId::LoginMissingCard);
        } else if (auto* client = data.application->apiClient()) {
            m_login_flow.start(*client, data.scanned_card->card_id);
        } else {
            m_local_error = Text(TextId::LoginMissingApi);
        }
    }

    Login::~Login() {
        getData().title_to_login_wipe = false;
        AudioAsset{ assets::bgm::Login }.stop();
    }

    void Login::update() {
        getData().title_to_login_wipe = false;
        updatePresentation();
        if (m_leaving) {
            return;
        }

        const auto* controller = getData().application->controller();
        if (controller && controller->maintenanceButton(CancelButton).down() && m_login_flow.canCancel() &&
            m_login_flow.state() != LoginState::UserFound) {
            returnToTitle();
            return;
        }

        const auto previous = m_login_flow.state();
        m_login_flow.update(getData().application->creditPool());
        if (m_credit_wait_timeout.update(m_login_flow.state() == LoginState::WaitingForCredit, Scene::DeltaTime())) {
            returnToTitle();
            return;
        }
        if (m_login_flow.state() != previous) {
            m_menu_input = {};
            m_repeat_action = RepeatAction::None;
            if (m_login_flow.state() == LoginState::RegistrationRequired) {
                m_step_elapsed = 0.0;
            }
        }
        const bool complete = m_login_flow.state() == LoginState::UserFound;
        switch (m_completion.update(complete, Scene::DeltaTime())) {
            case presentation::LoginCompletion::Action::Announce:
                audio::PlaySoundEffect(audio::SoundEffect::LoginComplete);
                break;
            case presentation::LoginCompletion::Action::Continue:
                startSession();
                return;
            case presentation::LoginCompletion::Action::None:
                break;
        }
        if (complete) {
            m_menu_input.update(controller, MenuKeys, Scene::DeltaTime(), false);
            return;
        }
        handleInput();
    }

    void Login::updatePresentation() {
        const double delta_seconds = Scene::DeltaTime();
        m_name_panel.update(delta_seconds);
        m_visibility_carousel.update(delta_seconds);
        m_step_elapsed = Min(StepTransitionSeconds, m_step_elapsed + delta_seconds);
        if (m_step_elapsed >= StepTransitionSeconds) {
            m_previous_step.reset();
        }
    }

    void Login::handleInput() {
        const bool registration = m_login_flow.state() == LoginState::RegistrationRequired;
        const bool enabled = m_login_flow.canCancel() && (!registration || m_step_elapsed >= StepTransitionSeconds);
        m_menu_input.update(
            getData().application->controller(),
            MenuKeys,
            Scene::DeltaTime(),
            enabled,
            registration && m_step == RegistrationStep::Name
        );
        if (!enabled) {
            m_repeat_action = RepeatAction::None;
            return;
        }
        if (!registration) {
            const bool failed = m_login_flow.state() == LoginState::Failed || !m_local_error.isEmpty();
            if (m_menu_input.touchDown(failed ? 0 : ReturnStart, failed ? 16 : ReturnWidth) ||
                (failed && m_menu_input.keyDown(KeySpace))) {
                returnToTitle();
            }
            return;
        }

        if (m_step == RegistrationStep::Name) {
            handleNameInput();
            return;
        }

        if (m_menu_input.touchDown(DoneStart, DoneWidth) || m_menu_input.keyDown(KeyDown)) {
            setStep(m_step == RegistrationStep::Confirm ? RegistrationStep::Visibility : RegistrationStep::Name);
            return;
        }
        if (m_step == RegistrationStep::Visibility) {
            const bool left = m_menu_input.keyDown(KeyLeft) || m_menu_input.touchDown(0, 3);
            const bool right = m_menu_input.keyDown(KeyRight) || m_menu_input.touchDown(3, 3);
            if ((left && m_visibility_index > 0) || (right && m_visibility_index == 0)) {
                m_visibility_index = right ? 1 : 0;
                m_visibility_carousel.animateMove(right ? 1 : -1);
                audio::PlaySoundEffect(audio::SoundEffect::Navigate);
            }
        }
        if (m_menu_input.keyDown(KeySpace) || m_menu_input.touchDown(6, 4)) {
            advanceRegistration();
        }
    }

    void Login::handleNameInput() {
        if (m_menu_input.touchDown(DoneStart, DoneWidth) || m_menu_input.keyDown(KeyUp)) {
            advanceRegistration();
            return;
        }
        if (m_menu_input.sideDown(SideButton::LeftUpper) || m_menu_input.sideDown(SideButton::LeftLower)) {
            changeCategory(-1);
            return;
        }
        if (m_menu_input.sideDown(SideButton::RightUpper) || m_menu_input.sideDown(SideButton::RightLower)) {
            changeCategory(1);
            return;
        }
        if (m_menu_input.keyDown(KeyDown) || m_menu_input.touchDown(DeleteStart, DeleteWidth)) {
            if (!m_name_entry.name().isEmpty()) {
                eraseCharacter();
                m_repeat_action = RepeatAction::Delete;
                m_repeat_elapsed = 0.0;
                m_next_repeat = 0.4;
            }
            return;
        }
        if (m_menu_input.keyDown(KeySpace) || m_menu_input.touchDown(6, 4)) {
            insertCharacter();
            return;
        }
        const bool left = m_menu_input.keyDown(KeyLeft) || m_menu_input.touchDown(0, 3);
        const bool right = m_menu_input.keyDown(KeyRight) || m_menu_input.touchDown(3, 3);
        if (left || right) {
            moveCharacter(right ? 1 : -1);
            m_repeat_action = right ? RepeatAction::Right : RepeatAction::Left;
            m_repeat_elapsed = 0.0;
            m_next_repeat = 0.4;
        } else {
            updateRepeat();
        }
    }

    void Login::updateRepeat() {
        bool held = false;
        switch (m_repeat_action) {
            case RepeatAction::Left:
                held = KeyLeft.pressed() || m_menu_input.touchPressed(0, 3);
                break;
            case RepeatAction::Right:
                held = KeyRight.pressed() || m_menu_input.touchPressed(3, 3);
                break;
            case RepeatAction::Delete:
                held = KeyDown.pressed() || m_menu_input.touchPressed(DeleteStart, DeleteWidth);
                break;
            case RepeatAction::None:
                return;
        }
        if (!held) {
            m_repeat_action = RepeatAction::None;
            return;
        }
        m_repeat_elapsed += Scene::DeltaTime();
        if (m_repeat_elapsed < m_next_repeat) {
            return;
        }
        m_next_repeat = m_repeat_elapsed + (m_repeat_elapsed >= 1.5 ? 0.055 : 0.11);
        if (m_repeat_action == RepeatAction::Delete) {
            eraseCharacter();
        } else {
            moveCharacter(m_repeat_action == RepeatAction::Right ? 1 : -1);
        }
    }

    void Login::moveCharacter(const int32 direction) {
        if (m_name_entry.move(direction)) {
            m_name_panel.animateMove(direction);
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
        }
    }

    void Login::changeCategory(const int32 direction) {
        if (m_name_entry.changeCategory(direction)) {
            m_name_panel = components::NameEntryPanel{};
            m_repeat_action = RepeatAction::None;
            audio::PlaySoundEffect(audio::SoundEffect::ChangeDifficulty);
        }
    }

    void Login::insertCharacter() {
        if (m_name_entry.appendSelected()) {
            audio::PlaySoundEffect(audio::SoundEffect::TextInput);
        }
    }

    void Login::eraseCharacter() {
        if (m_name_entry.eraseLast()) {
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
        }
    }

    void Login::advanceRegistration() {
        if (m_step == RegistrationStep::Name) {
            if (!m_name_entry.valid()) {
                return;
            }
            setStep(RegistrationStep::Visibility);
        } else if (m_step == RegistrationStep::Visibility) {
            setStep(RegistrationStep::Confirm);
        } else {
            m_login_flow.registerUser(m_name_entry.value(), m_visibility_index == 1);
            m_menu_input = {};
            m_repeat_action = RepeatAction::None;
            audio::PlaySoundEffect(audio::SoundEffect::Next);
        }
    }

    void Login::setStep(const RegistrationStep step) {
        const bool forward = step > m_step;
        m_previous_step = m_step;
        m_step = step;
        m_step_elapsed = 0.0;
        m_menu_input = {};
        m_repeat_action = RepeatAction::None;
        audio::PlaySoundEffect(forward ? audio::SoundEffect::Next : audio::SoundEffect::Back);
    }

    void Login::returnToTitle() {
        if (!m_login_flow.canCancel() || m_login_flow.state() == LoginState::UserFound) {
            return;
        }
        audio::PlaySoundEffect(audio::SoundEffect::Back);
        AudioAsset{ assets::bgm::Login }.stop(Duration{ 0.3 });
        m_leaving = changeScene(SceneState::Title, transitions::CardTransitionMillisec, CrossFade::No);
    }

    void Login::startSession() {
        auto& data = getData();
        const auto& config = data.application->config();
        const uint32 max_plays = config ? static_cast<uint32>(Max(0, config->system.playable)) : 0;
        data.application->playSession().start(*m_login_flow.user(), max_plays);
        data.application->playSession().setRecords(m_login_flow.records());
        if (const auto& catalog = m_login_flow.ratingCatalog()) {
            data.application->playSession().setRatingCatalog(*catalog);
        }
        data.application->playSession().setPlayOptions(*m_login_flow.playOptions());
        AudioAsset{ assets::bgm::Login }.stop(Duration{ 0.3 });
        m_leaving = changeScene(SceneState::MusicSelect, transitions::CardTransitionMillisec, CrossFade::No);
    }

    void Login::draw() const {
        drawScene({});
    }

    void Login::updateFadeIn(const double) {
        updatePresentation();
        m_menu_input.update(getData().application->controller(), MenuKeys, Scene::DeltaTime(), false);
    }

    void Login::updateFadeOut(const double) {
        updatePresentation();
    }

    void Login::drawFadeIn(const double t) const {
        if (getData().title_to_login_wipe) {
            drawScene({});
            transitions::DrawScreenWipe(0.5 + Clamp(t, 0.0, 1.0) * 0.5);
        } else {
            drawScene(transitions::CardFadeIn(t));
        }
    }

    void Login::drawFadeOut(const double t) const {
        drawScene(transitions::CardFadeOut(t));
    }

    void Login::drawScene(const transitions::CardTransition& transition) const {
        Scene::Rect().draw(theme::Palette::White);
        if (m_login_flow.state() == LoginState::RegistrationRequired) {
            auto step = m_step;
            auto animation = transition;
            if (transition.scale == 1.0 && transition.opacity == 1.0 && m_step_elapsed < StepTransitionSeconds) {
                const double progress = m_step_elapsed / StepTransitionSeconds;
                if (m_previous_step && progress < 0.5) {
                    step = *m_previous_step;
                    animation = transitions::CardFadeOut(progress * 2);
                } else {
                    animation = transitions::CardFadeIn(m_previous_step ? progress * 2 - 1 : Min(1.0, progress * 2));
                }
            }
            drawRegistration(step, animation);
        } else {
            const ScopedColorMul2D opacity{ ColorF{ 1.0, transition.opacity } };
            const Transformer2D transform{ Mat3x2::Scale(transition.scale, Vec2{ DesignSize.x / 2.0, 540 }) };
            drawStatus();
        }

        components::DrawMenuHeader(Assets::Header);
        if (m_login_flow.state() == LoginState::WaitingForCredit && !m_leaving) {
            components::DrawMenuTimerPlate(
                { .remaining_seconds = static_cast<int32>(Ceil(m_credit_wait_timeout.remainingSeconds())) },
                Point{ 1599, 72 }
            );
        }
        if (const auto& user = m_login_flow.user()) {
            const auto level = core::user::CalculateLevelProgress(user->xp);
            components::DrawUserNameplate(
                {
                    .display_name = user->display_name,
                    .rating = user->rating,
                    .level = level.level,
                    .level_progress = level.progress,
                },
                Point{ 59, 72 }
            );
        }
        if (m_login_flow.canCancel() && m_login_flow.state() != LoginState::UserFound && !m_leaving &&
            (m_login_flow.state() != LoginState::RegistrationRequired || m_step_elapsed >= StepTransitionSeconds)) {
            components::DrawSliderMappingGuide(sliderMappings(), RectF{ 210, 1010, 1500, 70 });
        }
    }

    void Login::drawRegistration(const RegistrationStep step, const transitions::CardTransition& transition) const {
        if (transition.scale <= 0.0 || transition.opacity <= 0.0) {
            return;
        }
        const ScopedColorMul2D opacity{ ColorF{ 1.0, transition.opacity } };
        const Transformer2D transform{ Mat3x2::Scale(transition.scale, Vec2{ DesignSize.x / 2.0, 540 }) };
        const TextId prompt = step == RegistrationStep::Name         ? TextId::LoginNamePrompt
                              : step == RegistrationStep::Visibility ? TextId::LoginVisibilityPrompt
                                                                     : TextId::LoginConfirmPrompt;
        FontAsset{ assets::font::Text }(Text(prompt))
            .drawAt(27, Vec2{ DesignSize.x / 2.0, 235 }, theme::Palette::BlueGray);
        switch (step) {
            case RegistrationStep::Name:
                m_name_panel.draw(m_name_entry);
                drawSideGuides();
                break;
            case RegistrationStep::Visibility:
                drawVisibility();
                break;
            case RegistrationStep::Confirm:
                drawConfirmation();
                break;
        }
    }

    void Login::drawVisibility() const {
        FontAsset{ assets::font::Text }(m_name_entry.value())
            .drawAt(46, Vec2{ DesignSize.x / 2.0, 305 }, theme::Palette::BlueGray);
        for (const auto& placement : m_visibility_carousel.layout(m_visibility_index, 2)) {
            const ColorF accent = placement.selected ? theme::Palette::Pink : theme::Palette::Cyan;
            const auto& region = placement.region;
            DrawPanel(region, accent);
            FontAsset{ assets::font::Label }(placement.index == 1 ? U"PUBLIC" : U"PRIVATE")
                .drawAt(36, region.center().movedBy(0, -45), accent);
            FontAsset{ assets::font::Text }(Text(placement.index == 1 ? TextId::LoginPublic : TextId::LoginPrivate))
                .drawAt(38, region.center().movedBy(0, 30), theme::Palette::BlueGray);
        }
        m_visibility_carousel.drawArrows(m_visibility_index, 2, theme::Palette::Gray);
        FontAsset{ assets::font::Text }(Text(TextId::LoginVisibilityDescription))
            .drawAt(24, Vec2{ DesignSize.x / 2.0, 820 }, theme::Palette::BlueGray);
    }

    void Login::drawConfirmation() const {
        DrawPanel(SummaryRegion, theme::Palette::Cyan);
        FontAsset{ assets::font::Text }(Text(TextId::LoginConfirmName))
            .drawAt(24, Vec2{ 960, 400 }, theme::Palette::BlueGray);
        FontAsset{ assets::font::Text }(m_name_entry.value()).drawAt(64, Vec2{ 960, 470 }, theme::Palette::BlueGray);
        Line{ 510, 540, 1410, 540 }.draw(1, theme::Palette::Pink, theme::Palette::Cyan);
        FontAsset{ assets::font::Text }(Text(TextId::LoginConfirmVisibility))
            .drawAt(24, Vec2{ 960, 590 }, theme::Palette::BlueGray);
        FontAsset{ assets::font::Text }(Text(m_visibility_index == 1 ? TextId::LoginPublic : TextId::LoginPrivate))
            .drawAt(36, Vec2{ 960, 645 }, theme::Palette::BlueGray);
    }

    void Login::drawStatus() const {
        DrawPanel(SummaryRegion, theme::Palette::Cyan);
        const bool failed = m_login_flow.error().has_value();
        FontAsset{ assets::font::Text }(statusText())
            .drawAt(44, SummaryRegion.center().movedBy(0, failed ? -55 : 0), theme::Palette::BlueGray);
        if (const auto& error = m_login_flow.error()) {
            FontAsset{ assets::font::Text }(error->message).draw(21, RectF{ 510, 535, 900, 150 }, theme::Palette::Gray);
        }
    }

    void Login::drawSideGuides() const {
        DrawSideLabel(false);
        DrawSideLabel(true);
    }

    Array<components::SliderMapping> Login::sliderMappings() const {
        using components::SliderMapping;
        const auto mapping = [](const uint32 start,
                                const uint32 width,
                                const TextId text,
                                const ColorF& color,
                                const bool enabled = true) {
            return SliderMapping{
                .region = { .start = start, .width = width },
                .label = Text(text),
                .color = enabled ? color : theme::Palette::Gray,
            };
        };
        if (m_login_flow.state() != LoginState::RegistrationRequired) {
            const bool failed = m_login_flow.state() == LoginState::Failed || !m_local_error.isEmpty();
            return { mapping(
                failed ? 0 : ReturnStart,
                failed ? 16 : ReturnWidth,
                TextId::LoginReturnTitle,
                theme::Palette::Purple
            ) };
        }
        const bool editing_name = m_step == RegistrationStep::Name;
        Array<SliderMapping> mappings;
        if (m_step != RegistrationStep::Confirm) {
            const bool can_left = editing_name ? m_name_entry.selectedIndex() > 0 : m_visibility_index > 0;
            const bool can_right = editing_name ? m_name_entry.selectedIndex() + 1 < m_name_entry.characters().size()
                                                : m_visibility_index == 0;
            mappings.push_back(mapping(0, 3, TextId::LoginMoveLeft, theme::Palette::Pink, can_left));
            mappings.push_back(mapping(3, 3, TextId::LoginMoveRight, theme::Palette::Pink, can_right));
        }
        mappings.push_back(mapping(
            6,
            4,
            m_step == RegistrationStep::Name      ? TextId::LoginInsert
            : m_step == RegistrationStep::Confirm ? TextId::LoginRegister
                                                  : TextId::LoginNext,
            theme::Palette::Cyan,
            !editing_name || m_name_entry.length() < input::NameEntry::MaximumLength
        ));
        if (editing_name) {
            mappings.push_back(mapping(
                DeleteStart,
                DeleteWidth,
                TextId::LoginDelete,
                theme::Palette::Pink,
                !m_name_entry.name().isEmpty()
            ));
        }
        mappings.push_back(mapping(
            DoneStart,
            DoneWidth,
            editing_name ? TextId::LoginDone : TextId::LoginBack,
            theme::Palette::Purple,
            !editing_name || m_name_entry.valid()
        ));
        return mappings;
    }

    String Login::statusText() const {
        if (!m_local_error.isEmpty()) {
            return m_local_error;
        }
        switch (m_login_flow.state()) {
            case LoginState::WaitingForCredit:
                return Text(TextId::LoginWaitingForCredit);
            case LoginState::RegisteringUser:
            case LoginState::ConfirmingRegistration:
                return Text(TextId::LoginRegistering);
            case LoginState::UserFound:
                return Text(m_completion.announced() ? TextId::LoginComplete : TextId::LoginLoadingUser);
            case LoginState::Failed:
                return Text(TextId::LoginFailed);
            default:
                return Text(TextId::LoginLoadingUser);
        }
    }

    void Login::RegisterAssets() {
        input::NameEntry entry;
        String latin_chars;
        String cjk_chars;
        for (const char32 character : entry.characters()) {
            (character < 0x80 ? latin_chars : cjk_chars).push_back(character);
        }
        latin_chars = latin_chars.sorted_and_uniqued();
        if (!FontAsset{ assets::font::Text }.preload(latin_chars) ||
            !FontAsset{ assets::font::Label }.preload(latin_chars) ||
            !FontAsset{ assets::font::CjkFallback }.preload(cjk_chars.sorted_and_uniqued())) {
            throw Error{ U"Failed to preload the Login character glyphs." };
        }
        if (!TextureAsset::Register(
                Assets::Header,
                components::MakeMenuHeaderTexture(U"LOGIN", Text(TextId::LoginPrompt))
            ) ||
            !TextureAsset::Load(Assets::Header)) {
            throw Error{ U"Failed to register the Login header texture." };
        }
    }
}
