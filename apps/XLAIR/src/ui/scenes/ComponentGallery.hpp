#pragma once

#include "ui/components/MusicCard.hpp"
#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class ComponentGallery final : public SceneBase {
    public:
        explicit ComponentGallery(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        mutable components::MusicCard m_music_card;
        Texture m_jacket;
        double m_elapsed = 0.0;
    };
}
