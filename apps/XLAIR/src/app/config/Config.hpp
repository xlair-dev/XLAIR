#pragma once

#include "Common.hpp"

namespace xlair::app {
    struct Config {
        struct CardReader {
            enum class Mode {
                Mock,
                Pasori,
            };

            Mode mode = Mode::Mock;

            struct Mock {
                String card_id = U"00000000";
            } mock;
        } card_reader;

        struct Api {
            enum class Mode { Http, Mock };

            Mode mode = Mode::Http;
            URL endpoint = U"https://api.xlair.dev";
            double timeout_seconds = 10.0;

            struct Mock {
                FilePath data_directory;
            } mock;

            struct Auth {
                String domain;
                String client_id;
                String client_secret;
                String audience = U"https://api.xlair.dev";
            } auth;

            [[nodiscard]]
            inline URL syncSource() const {
                return mode == Mode::Mock ? U"mock://" + mock.data_directory : endpoint;
            }
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

        struct Controller {
            enum class Mode {
                Keyboard,
                GroundSlider,
                V1,
            };

            Mode mode = Mode::Keyboard;

            struct GroundSlider {
                String port;
                int32 baud_rate = 115200;
                uint8 touch_threshold = 1;
            } ground_slider;

            struct V1 {
                String port;
                int32 baud_rate = 115200;
            } v1;
        } controller;

        struct Input {
            double latency_offset_seconds = 0.0;
        } input;
    };
}
