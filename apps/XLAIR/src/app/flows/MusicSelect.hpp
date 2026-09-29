#pragma once

#include "Common.hpp"

#include <SheetsAnalyzer/Metadata.hpp>

namespace xlair::app::flows {
    class MusicSelect {
    public:
        explicit MusicSelect(const Array<sheets::Metadata>& catalog);

        [[nodiscard]]
        bool moveMusic(int32 direction) noexcept;

        [[nodiscard]]
        bool moveDifficulty(int32 direction) noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        [[nodiscard]]
        std::size_t musicCount() const noexcept;

        [[nodiscard]]
        std::size_t selectedIndex() const noexcept;

        [[nodiscard]]
        const sheets::Metadata* musicAt(std::size_t index) const noexcept;

        [[nodiscard]]
        const sheets::Metadata* selectedMusic() const noexcept;

        [[nodiscard]]
        const sheets::Difficulty* selectedDifficulty() const noexcept;

        [[nodiscard]]
        const sheets::Difficulty* difficultyFor(const sheets::Metadata& music) const noexcept;

    private:
        void normalizeDifficulty() noexcept;

        const Array<sheets::Metadata>* m_catalog = nullptr;
        std::size_t m_selected_index = 0;
        uint32 m_difficulty_code = 0;
    };
}
