#pragma once

#include "app/flows/Login.hpp"
#include "ui/Scene.hpp"
#include "ui/components/NameEntryPanel.hpp"
#include "ui/components/SliderMappingGuide.hpp"
#include "ui/input/MenuInput.hpp"
#include "ui/presentation/LoginCompletion.hpp"
#include "ui/transitions/CardTransition.hpp"

namespace xlair::ui::scenes {
    class Login final : public SceneBase {
    public:
        explicit Login(const InitData& init);
        ~Login() override;

        void update() override;
        void draw() const override;
        void updateFadeIn(double t) override;
        void updateFadeOut(double t) override;
        void drawFadeIn(double t) const override;
        void drawFadeOut(double t) const override;

        static void RegisterAssets();

    private:
        enum class RegistrationStep { Name, Visibility, Confirm };
        enum class RepeatAction { None, Left, Right, Delete };

        struct Assets {
            static constexpr AssetNameView Header{ U"XLAIR.Login.Header" };
        };

        void updatePresentation();
        void handleInput();
        void handleNameInput();
        void updateRepeat();
        void moveCharacter(int32 direction);
        void changeCategory(int32 direction);
        void insertCharacter();
        void eraseCharacter();
        void advanceRegistration();
        void setStep(RegistrationStep step);
        void returnToTitle();
        void startSession();
        void drawScene(const transitions::CardTransition& transition) const;
        void drawRegistration(RegistrationStep step, const transitions::CardTransition& transition) const;
        void drawVisibility() const;
        void drawConfirmation() const;
        void drawStatus() const;
        void drawSideGuides() const;

        [[nodiscard]]
        Array<components::SliderMapping> sliderMappings() const;

        [[nodiscard]]
        String statusText() const;

        app::flows::Login m_login_flow;
        input::NameEntry m_name_entry;
        input::MenuInput m_menu_input;
        components::NameEntryPanel m_name_panel;
        components::CardCarousel m_visibility_carousel;
        presentation::LoginCompletion m_completion;
        RegistrationStep m_step = RegistrationStep::Name;
        Optional<RegistrationStep> m_previous_step;
        std::size_t m_visibility_index = 0;
        RepeatAction m_repeat_action = RepeatAction::None;
        double m_repeat_elapsed = 0.0;
        double m_next_repeat = 0.4;
        double m_step_elapsed = 0.0;
        String m_local_error;
        bool m_leaving = false;
    };
}
