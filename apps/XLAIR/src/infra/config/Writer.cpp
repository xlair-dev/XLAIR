#include "Writer.hpp"

#include "Loader.hpp"

#include <filesystem>

namespace xlair::infra::config {
    namespace {
        class ConfigWriter {
        public:
            // Keep full keys grouped by table; TOML tables must not be declared twice.
            template <class Type> ConfigWriter& write(const StringView key, const Type& value) {
                const auto separator = key.lastIndexOf(U'.');
                const auto section = key.substr(0, separator);
                if (section != m_section) {
                    if (!m_text.isEmpty()) {
                        m_text += U'\n';
                    }
                    m_text += U"[{}]\n"_fmt(section);
                    m_section = section;
                }

                // JSON scalar literals provide TOML-compatible escaping and numeric precision.
                m_text += U"{} = {}\n"_fmt(key.substr(separator + 1), JSON(value).formatMinimum());
                return *this;
            }

            [[nodiscard]]
            const String& text() const noexcept {
                return m_text;
            }

        private:
            String m_section;
            String m_text;
        };

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
            const StringView controller_mode =
                config.controller.mode == app::Config::Controller::Mode::Keyboard       ? U"keyboard"
                : config.controller.mode == app::Config::Controller::Mode::GroundSlider ? U"ground_slider"
                                                                                        : U"v1";
            const StringView card_reader_mode =
                config.card_reader.mode == app::Config::CardReader::Mode::Mock ? U"mock" : U"pasori";
            const StringView api_mode = config.api.mode == app::Config::Api::Mode::Http ? U"http" : U"mock";

            ConfigWriter writer;
            writer
                // system
                .write(U"system.arcade", config.system.arcade)
                .write(U"system.autoplay", config.system.autoplay)
                .write(U"system.playable", config.system.playable)
                .write(U"system.menu_timer_seconds", config.system.menu_timer_seconds)
                // window
                .write(U"window.width", config.window.width)
                .write(U"window.height", config.window.height)
                .write(U"window.sizable", config.window.sizable)
                .write(U"window.fullscreen", config.window.fullscreen)
                .write(U"window.letterbox_color", letterbox_color)
                // audio
                .write(U"audio.music_volume", config.audio.music_volume)
                .write(U"audio.ui_sound_volume", config.audio.ui_sound_volume)
                .write(U"audio.gameplay_sound_volume", config.audio.gameplay_sound_volume)
                // controller
                .write(U"controller.mode", controller_mode)
                .write(U"controller.highlight_touch_threshold", config.controller.highlight_touch_threshold)
                .write(U"controller.ground_slider.port", config.controller.ground_slider.port)
                .write(U"controller.ground_slider.baud_rate", config.controller.ground_slider.baud_rate)
                .write(U"controller.ground_slider.touch_threshold", config.controller.ground_slider.touch_threshold)
                .write(U"controller.v1.port", config.controller.v1.port)
                .write(U"controller.v1.baud_rate", config.controller.v1.baud_rate)
                .write(U"controller.v1.touch_threshold", config.controller.v1.touch_threshold)
                // input
                .write(U"input.latency_offset_seconds", config.input.latency_offset_seconds)
                // sync
                .write(U"sync.directory", config.sync.directory)
                // card reader
                .write(U"card_reader.mode", card_reader_mode)
                .write(U"card_reader.mock.card_id", config.card_reader.mock.card_id)
                // api
                .write(U"api.mode", api_mode)
                .write(U"api.endpoint", config.api.endpoint)
                .write(U"api.timeout_seconds", config.api.timeout_seconds)
                .write(U"api.mock.data_directory", config.api.mock.data_directory)
                .write(U"api.auth.domain", config.api.auth.domain)
                .write(U"api.auth.client_id", config.api.auth.client_id)
                .write(U"api.auth.client_secret", config.api.auth.client_secret)
                .write(U"api.auth.audience", config.api.auth.audience);
            return writer.text();
        }

        void WriteFile(const FilePath& path, const String& text, const String& source) {
            const auto target = std::filesystem::canonical(std::filesystem::path{ path.toUTF32() });
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
            if (TextReader{ path }.readAll() != source) {
                throw Error{ U"The config file changed while saving. Please try again." };
            }
            std::filesystem::rename(temporary, target);
            cleanup.dismiss();
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
            WriteFile(m_path, Serialize(config, letterbox_color), source);
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
