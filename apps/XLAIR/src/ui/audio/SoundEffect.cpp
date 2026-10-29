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
                case SoundEffect::Finish:
                    return { assets::sound::Finish, UISound };
                case SoundEffect::CoinInsert:
                    return { assets::sound::CoinInsert, UISound };
                case SoundEffect::LoginComplete:
                    return { assets::sound::LoginComplete, UISound };
                case SoundEffect::TimerNormal:
                    return { assets::sound::TimerNormal, UISound };
            }

            throw Error{ U"Unknown sound effect." };
        }
    }

    void PlaySoundEffect(const SoundEffect sound) {
        const auto [asset, bus] = GetDefinition(sound);
        AudioAsset{ asset }.playOneShot(bus);
    }
}
