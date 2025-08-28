#pragma once
#include "Common.hpp"

namespace core::types {
    struct Config {
        struct Window {
            int32 width;
            int32 height;
            bool sizable;
            bool fullscreen;
            ColorF letterbox_color;
        } window;

        struct System {
            bool arcade;
        } system;
    };
}
