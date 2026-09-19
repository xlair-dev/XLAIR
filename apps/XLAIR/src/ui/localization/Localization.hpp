#pragma once

#include "Common.hpp"

namespace xlair::ui::localization {
    enum class TextId {
#define X(name, key) name,
#include "TextList.inc"
#undef X
        Count,
    };

    void Initialize();

    [[nodiscard]]
    const String& GetText(TextId id);
}
