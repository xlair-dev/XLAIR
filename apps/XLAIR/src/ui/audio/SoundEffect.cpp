#include "SoundEffect.hpp"

#include "AudioBus.hpp"
#include "ui/assets/Assets.hpp"

namespace xlair::ui::audio {
    namespace {
        struct SoundEffectDefinition {
            AssetNameView asset;
            MixBus bus;
        };

        [[nodiscard]]
        SoundEffectDefinition GetDefinition(const SoundEffect sound) {
            switch (sound) {
                case SoundEffect::Navigate:
                    return { assets::sound::Navigate, UISound };
                case SoundEffect::ChangeDifficulty:
                    return { assets::sound::ChangeDifficulty, UISound };
                case SoundEffect::Confirm:
                    return { assets::sound::Confirm, UISound };
            }

            throw Error{ U"Unknown sound effect." };
        }
    }

    void PlaySoundEffect(const SoundEffect sound) {
        const auto [asset, bus] = GetDefinition(sound);
        AudioAsset{ asset }.playOneShot(bus);
    }
}
