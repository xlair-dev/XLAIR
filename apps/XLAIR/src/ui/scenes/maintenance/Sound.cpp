#include "Sound.hpp"

#include "ui/assets/Assets.hpp"
#include "ui/audio/AudioBus.hpp"

#include <array>
#include <cmath>

namespace xlair::ui::scenes::maintenance {
    namespace {
        constexpr std::array<StringView, 4> SoundItems{ U"楽曲", U"UI SE", U"判定・ホールド SE", U"戻る" };
        constexpr std::array SoundVolumes{
            &app::Config::Audio::music_volume,
            &app::Config::Audio::ui_sound_volume,
            &app::Config::Audio::gameplay_sound_volume,
        };
    }

    Sound::Sound(const InitData& init) : Base{ init } {}

    void Sound::update() {
        if (!navigate()) {
            return;
        }
        if (UpdateMenuSelection(m_selected_index, SoundItems)) {
            m_value_repeat.reset();
        }
        if (m_selected_index == SoundVolumes.size()) {
            m_value_repeat.reset();
            if (KeyEnter.down()) {
                changeScene(SceneState::Maintenance, 0);
            }
            return;
        }
        const int32 delta = m_value_repeat.delta(100);
        if (delta == 0) {
            return;
        }
        auto& application = *getData().application;
        const auto& config = application.config();
        if (!config) {
            return;
        }
        auto volumes = config->audio;
        auto& value = volumes.*SoundVolumes[m_selected_index];
        const int32 volume = static_cast<int32>(std::lround(value * 100.0));
        value = Clamp(volume + delta, 0, 100) / 100.0;
        if (application.setAudioVolumes(volumes)) {
            audio::ApplyVolumes(config->audio);
        }
    }

    void Sound::draw() const {
        drawFrame(U"サウンド", U"↑ / ↓: 選択    ← / →: 音量（長押し可）    F2: Title");
        DrawMenu(SoundItems, m_selected_index);
        const auto& config = getData().application->config();
        if (!config) {
            return;
        }
        const FontAsset font{ assets::font::Text };
        for (std::size_t index = 0; index < SoundVolumes.size(); ++index) {
            const auto volume = static_cast<int32>(std::lround(config->audio.*SoundVolumes[index] * 100.0));
            font(U"{}%"_fmt(volume)).draw(28, Vec2{ 480, 128.0 + index * 52.0 }, Palette::White);
        }
    }
}
