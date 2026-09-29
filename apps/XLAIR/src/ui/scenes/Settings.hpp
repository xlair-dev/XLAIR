#pragma once

#include "ui/Scene.hpp"
#include "ui/components/SliderMappingGuide.hpp"

namespace xlair::ui::scenes {
    class Settings final : public SceneBase {
    public:
        explicit Settings(const InitData& init);

        void update() override;
        void draw() const override;

        static void RegisterAssets();

    private:
        struct Assets {
            static constexpr AssetNameView Header{ U"XLAIR.Settings.Header" };
        };

        Array<components::SliderMapping> m_slider_mappings;
    };
}
