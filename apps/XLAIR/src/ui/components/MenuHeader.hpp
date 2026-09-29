#pragma once

#include "Common.hpp"

#include <memory>

namespace xlair::ui::components {
    [[nodiscard]]
    std::unique_ptr<TextureAssetData> MakeMenuHeaderTexture(String title, String subtitle);

    void DrawMenuHeader(AssetNameView header);
}
