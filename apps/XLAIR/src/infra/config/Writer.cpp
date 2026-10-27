#include "Writer.hpp"

#include "Loader.hpp"

#include <filesystem>

namespace xlair::infra::config {
    namespace {
        void ApplyChanges(app::Config& config, const app::Config& saved, const app::Config& current) {
            const auto update = [](auto& destination, const auto before, const auto after) {
                if (before != after) {
                    destination = after;
                }
            };
            update(config.system.autoplay, saved.system.autoplay, current.system.autoplay);
            update(config.audio.music_volume, saved.audio.music_volume, current.audio.music_volume);
            update(config.audio.ui_sound_volume, saved.audio.ui_sound_volume, current.audio.ui_sound_volume);
            update(
                config.audio.gameplay_sound_volume,
                saved.audio.gameplay_sound_volume,
                current.audio.gameplay_sound_volume
            );
            update(
                config.controller.highlight_touch_threshold,
                saved.controller.highlight_touch_threshold,
                current.controller.highlight_touch_threshold
            );
            update(
                config.controller.ground_slider.touch_threshold,
                saved.controller.ground_slider.touch_threshold,
                current.controller.ground_slider.touch_threshold
            );
            update(
                config.controller.v1.touch_threshold,
                saved.controller.v1.touch_threshold,
                current.controller.v1.touch_threshold
            );
            update(
                config.input.latency_offset_seconds,
                saved.input.latency_offset_seconds,
                current.input.latency_offset_seconds
            );
            update(config.api.timeout_seconds, saved.api.timeout_seconds, current.api.timeout_seconds);
        }

        String Serialize(const app::Config& config, const String& letterbox_color) {
            String text;
            const auto section = [&](const StringView name) {
                if (!text.isEmpty()) {
                    text += U'\n';
                }
                text += U"[{}]\n"_fmt(name);
            };
            const auto write = [&](const StringView name, const auto& value) {
                // JSON scalar literals also provide the escaping and full numeric
                // precision needed by these TOML strings, booleans and numbers.
                text += U"{} = {}\n"_fmt(name, JSON(value).formatMinimum());
            };

            section(U"system");
            write(U"arcade", config.system.arcade);
            write(U"autoplay", config.system.autoplay);
            write(U"playable", config.system.playable);
            write(U"menu_timer_seconds", config.system.menu_timer_seconds);

            section(U"window");
            write(U"width", config.window.width);
            write(U"height", config.window.height);
            write(U"sizable", config.window.sizable);
            write(U"fullscreen", config.window.fullscreen);
            write(U"letterbox_color", letterbox_color);

            section(U"audio");
            write(U"music_volume", config.audio.music_volume);
            write(U"ui_sound_volume", config.audio.ui_sound_volume);
            write(U"gameplay_sound_volume", config.audio.gameplay_sound_volume);

            section(U"input");
            write(U"latency_offset_seconds", config.input.latency_offset_seconds);

            section(U"sync");
            write(U"directory", config.sync.directory);

            section(U"controller");
            write(
                U"mode",
                config.controller.mode == app::Config::Controller::Mode::Keyboard       ? U"keyboard"
                : config.controller.mode == app::Config::Controller::Mode::GroundSlider ? U"ground_slider"
                                                                                        : U"v1"
            );
            write(U"highlight_touch_threshold", config.controller.highlight_touch_threshold);

            section(U"controller.ground_slider");
            write(U"port", config.controller.ground_slider.port);
            write(U"baud_rate", config.controller.ground_slider.baud_rate);
            write(U"touch_threshold", config.controller.ground_slider.touch_threshold);

            section(U"controller.v1");
            write(U"port", config.controller.v1.port);
            write(U"baud_rate", config.controller.v1.baud_rate);
            write(U"touch_threshold", config.controller.v1.touch_threshold);

            section(U"card_reader");
            write(U"mode", config.card_reader.mode == app::Config::CardReader::Mode::Mock ? U"mock" : U"pasori");

            section(U"card_reader.mock");
            write(U"card_id", config.card_reader.mock.card_id);

            section(U"api");
            write(U"mode", config.api.mode == app::Config::Api::Mode::Http ? U"http" : U"mock");
            write(U"endpoint", config.api.endpoint);
            write(U"timeout_seconds", config.api.timeout_seconds);

            section(U"api.mock");
            write(U"data_directory", config.api.mock.data_directory);

            section(U"api.auth");
            write(U"domain", config.api.auth.domain);
            write(U"client_id", config.api.auth.client_id);
            write(U"client_secret", config.api.auth.client_secret);
            write(U"audience", config.api.auth.audience);
            return text;
        }

    }

    Writer::Writer(FilePath path, FilePath local_sheets_directory)
        : m_path{ std::move(path) }, m_local_sheets_directory{ std::move(local_sheets_directory) } {}

    Optional<app::interfaces::ConfigSaveError>
    Writer::save(const app::Config& saved, const app::Config& current) const {
        try {
            if (saved == current) {
                return none;
            }
            const String source = TextReader{ m_path }.readAll();
            auto loaded = Loader{ m_path, m_local_sheets_directory }.load();
            if (!loaded) {
                return app::interfaces::ConfigSaveError{ loaded.error->message, m_path };
            }
            auto config = std::move(*loaded.value);
            const TOMLReader original{ m_path };
            if (!original) {
                throw Error{ U"Could not read the config file." };
            }

            // Loader resolves paths for runtime use; keep their original spelling on disk.
            const app::Config defaults;
            config.sync.directory = original[U"sync.directory"].getOr<String>(defaults.sync.directory);
            config.api.mock.data_directory =
                original[U"api.mock.data_directory"].getOr<String>(defaults.api.mock.data_directory);
            const auto letterbox_color = original[U"window.letterbox_color"].getOr<String>(String{ U"#000000" });
            ApplyChanges(config, saved, current);
            const String text = Serialize(config, letterbox_color);

            const auto target = std::filesystem::canonical(std::filesystem::path{ m_path.toUTF32() });
            const FilePath target_path{ target.u32string() };
            const FilePath temporary_path = FileSystem::UniqueFilePath(FileSystem::ParentPath(target_path));
            ScopeGuard cleanup{ [&] {
                FileSystem::Remove(temporary_path);
            } };
            const std::filesystem::path temporary{ temporary_path.toUTF32() };
            TextWriter writer{ temporary_path, TextEncoding::UTF8_NO_BOM };
            if (!writer) {
                throw Error{ U"Could not create a temporary config file." };
            }
            std::filesystem::permissions(temporary, std::filesystem::status(target).permissions());
            writer.write(text);
            writer.close();
            if (TextReader{ temporary_path }.readAll() != text || !TOMLReader{ temporary_path }) {
                throw Error{ U"Could not write and validate the complete config file." };
            }
            if (TextReader{ m_path }.readAll() != source) {
                throw Error{ U"The config file changed while saving. Please try again." };
            }
            std::filesystem::rename(temporary, target);
            cleanup.dismiss();
            return none;
        } catch (const Error& error) {
            return app::interfaces::ConfigSaveError{ error.what(), m_path };
        } catch (const std::exception&) {
            return app::interfaces::ConfigSaveError{
                U"Could not save the config file. Check file permissions and available disk space.",
                m_path
            };
        }
    }
}
