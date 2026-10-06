#pragma once

#include "ui/components/GameScoreBar.hpp"
#include "ui/components/MusicCard.hpp"
#include "ui/components/SettingCard.hpp"
#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class ComponentGallery final : public SceneBase {
    public:
        explicit ComponentGallery(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        void drawGameHud() const;

        mutable components::MusicCard m_music_card;
        mutable components::SettingCard m_setting_card;
        mutable components::GameScoreBar m_game_score_bar;
        Texture m_jacket;
        double m_elapsed = 0.0;
    };
}
