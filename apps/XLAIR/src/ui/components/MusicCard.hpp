#pragma once

#include "Common.hpp"
#include "ui/theme/DifficultyTheme.hpp"

namespace xlair::ui::components {
    // Text fields are non-owning and must remain valid until MusicCard::render() returns.
    struct MusicCardData {
        StringView title;
        StringView artist;
        StringView difficulty;
        double level = 0.0;
        StringView designer;
        uint32 high_score = 0;
        StringView grade;
        StringView clear_status;
        bool available = true;
    };

    class MusicCard {
    public:
        MusicCard();

        [[nodiscard]]
        const MSRenderTexture& render(
            const MusicCardData& data,
            const TextureRegion& jacket,
            const theme::DifficultyTheme& difficulty_theme,
            double text_offset = 0.0
        );

        [[nodiscard]]
        static constexpr Size size() noexcept {
            return CardSize;
        }

    private:
        void drawBackground(const TextureRegion& jacket, const theme::DifficultyTheme& difficulty_theme) const;
        void drawDifficulty(const MusicCardData& data, const theme::DifficultyTheme& difficulty_theme) const;
        void drawMetadata(
            const MusicCardData& data,
            const theme::DifficultyTheme& difficulty_theme,
            double text_offset
        ) const;
        void drawUnavailable() const;
        void drawBadge(const Vec2& position, const Vec2& size, StringView text, const ColorF& color) const;

        static constexpr Size CardSize{ 416, 545 };

        MSRenderTexture m_render_texture;
    };
}
