#pragma once

#include "Common.hpp"

namespace xlair::app {
    struct Config {
        struct Sync {
            FilePath directory = U"data/sheets/sync";

            bool operator==(const Sync&) const = default;
        } sync;

        struct CardReader {
            enum class Mode {
                Mock,
                Pasori,
            };

            Mode mode = Mode::Mock;

            struct Mock {
                String card_id = U"00000000";

                bool operator==(const Mock&) const = default;
            } mock;

            bool operator==(const CardReader&) const = default;
        } card_reader;

        struct Api {
            enum class Mode { Http, Mock };

            Mode mode = Mode::Http;
            URL endpoint = U"https://api.xlair.dev";
            double timeout_seconds = 10.0;

            struct Mock {
                FilePath data_directory;

                bool operator==(const Mock&) const = default;
            } mock;

            struct Auth {
                String domain;
                String client_id;
                String client_secret;
                String audience = U"https://api.xlair.dev";

                bool operator==(const Auth&) const = default;
            } auth;

            bool operator==(const Api&) const = default;

            [[nodiscard]]
            inline URL syncSource() const {
                return mode == Mode::Mock ? U"mock://" + mock.data_directory : endpoint;
            }
        } api;

        struct System {
            bool arcade = false;
            bool autoplay = false;
            int32 playable = 3;
            int32 menu_timer_seconds = 90;

            bool operator==(const System&) const = default;
        } system;

        struct Window {
            int32 width = 1920;
            int32 height = 1080;
            bool sizable = true;
            bool fullscreen = false;
            ColorF letterbox_color{ 0.0 };

            bool operator==(const Window&) const = default;
        } window;

        struct Audio {
            double music_volume = 1.0;
            double ui_sound_volume = 1.0;
            double gameplay_sound_volume = 0.55;

            bool operator==(const Audio&) const = default;
        } audio;

        struct Controller {
            enum class Mode {
                Keyboard,
                GroundSlider,
                V1,
            };

            Mode mode = Mode::Keyboard;
            bool highlight_touch_threshold = false;

            struct GroundSlider {
                String port;
                int32 baud_rate = 115200;
                uint8 touch_threshold = 1;

                bool operator==(const GroundSlider&) const = default;
            } ground_slider;

            struct V1 {
                String port = U"auto";
                int32 baud_rate = 115200;
                uint8 touch_threshold = 1;

                bool operator==(const V1&) const = default;
            } v1;

            bool operator==(const Controller&) const = default;
        } controller;

        struct Input {
            double latency_offset_seconds = 0.0;

            bool operator==(const Input&) const = default;
        } input;

        bool operator==(const Config&) const = default;
    };
}
