#include "Addon.hpp"
#include "Common.hpp"

// di
#include "ui/addons/HomographyAddon.hpp"
#include "ui/addons/TextShadowAddon.hpp"

namespace app {
    void RegisterAddons() {
        Addon::Register<ui::addons::HomographyAddon>(ui::addons::HomographyAddon::Name);
        Addon::Register<ui::addons::TextShadowAddon>(ui::addons::TextShadowAddon::Name);
    }
}
