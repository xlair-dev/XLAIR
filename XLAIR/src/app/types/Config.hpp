#pragma once
#include "Common.hpp"
#include "app/consts/Config.hpp"
#include "app/interfaces/ISheetProvider.hpp"
#include "app/interfaces/IController.hpp"

namespace app::types {
    struct Config {
        struct System {
            bool arcade = consts::defaults::Arcade;
        } system;

        struct Window {
            int32 width = consts::defaults::Width;
            int32 height = consts::defaults::Height;
            bool sizable = consts::defaults::Sizable;
            bool fullscreen = consts::defaults::Fullscreen;
            ColorF letterbox_color = consts::defaults::LetterboxColor;
        } window;

        std::shared_ptr<interfaces::ISheetProvider> sheet_provider;
        std::shared_ptr<interfaces::IController> controller;
    };
}
