#pragma once

#include "Common.hpp"

namespace xlair::app {
    struct Config {
        struct Api {
            URL endpoint = U"https://api.xlair.dev";
            double timeout_seconds = 10.0;

            struct Auth {
                String domain;
                String client_id;
                String client_secret;
                String audience = U"https://api.xlair.dev";
            } auth;
        } api;

        struct System {
            bool arcade = false;
            int32 playable = 3;
            int32 menu_timer_seconds = 90;
        } system;

        struct Window {
            int32 width = 1920;
            int32 height = 1080;
            bool sizable = true;
            bool fullscreen = false;
            ColorF letterbox_color{ 0.0 };
        } window;

        struct Input {
            double latency_offset_seconds = 0.0;
        } input;
    };
}
