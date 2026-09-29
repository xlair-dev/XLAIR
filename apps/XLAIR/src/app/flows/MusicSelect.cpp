#include "MusicSelect.hpp"

#include <limits>

namespace xlair::app::flows {
    MusicSelect::MusicSelect(const Array<sheets::Metadata>& catalog) : m_catalog{ &catalog } {
        normalizeDifficulty();
    }

    bool MusicSelect::moveMusic(const int32 direction) noexcept {
        if (empty() || direction == 0) {
            return false;
        }

        const int64 destination = static_cast<int64>(m_selected_index) + (direction < 0 ? -1 : 1);
        if (destination < 0 || destination >= static_cast<int64>(m_catalog->size())) {
            return false;
        }

        m_selected_index = static_cast<std::size_t>(destination);
        normalizeDifficulty();
        return true;
    }

    bool MusicSelect::moveDifficulty(const int32 direction) noexcept {
        const auto* music = selectedMusic();
        if (!music || music->difficulties.isEmpty() || direction == 0) {
            return false;
        }

        std::size_t current = 0;
        for (std::size_t index = 0; index < music->difficulties.size(); ++index) {
            if (music->difficulties[index].index == m_difficulty_code) {
                current = index;
                break;
            }
        }

        const int64 destination = static_cast<int64>(current) + (direction < 0 ? -1 : 1);
        if (destination < 0 || destination >= static_cast<int64>(music->difficulties.size())) {
            return false;
        }

        m_difficulty_code = music->difficulties[static_cast<std::size_t>(destination)].index;
        return true;
    }

    bool MusicSelect::empty() const noexcept {
        return !m_catalog || m_catalog->isEmpty();
    }

    std::size_t MusicSelect::musicCount() const noexcept {
        return m_catalog ? m_catalog->size() : 0;
    }

    std::size_t MusicSelect::selectedIndex() const noexcept {
        return m_selected_index;
    }

    const sheets::Metadata* MusicSelect::musicAt(const std::size_t index) const noexcept {
        if (!m_catalog || index >= m_catalog->size()) {
            return nullptr;
        }
        return &(*m_catalog)[index];
    }

    const sheets::Metadata* MusicSelect::selectedMusic() const noexcept {
        return musicAt(m_selected_index);
    }

    const sheets::Difficulty* MusicSelect::selectedDifficulty() const noexcept {
        const auto* music = selectedMusic();
        return music ? difficultyFor(*music) : nullptr;
    }

    const sheets::Difficulty* MusicSelect::difficultyFor(const sheets::Metadata& music) const noexcept {
        if (music.difficulties.isEmpty()) {
            return nullptr;
        }

        const sheets::Difficulty* nearest = &music.difficulties.front();
        uint32 nearest_distance = std::numeric_limits<uint32>::max();
        for (const auto& difficulty : music.difficulties) {
            if (difficulty.index == m_difficulty_code) {
                return &difficulty;
            }

            const uint32 distance = difficulty.index > m_difficulty_code ? difficulty.index - m_difficulty_code
                                                                         : m_difficulty_code - difficulty.index;
            if (distance < nearest_distance) {
                nearest = &difficulty;
                nearest_distance = distance;
            }
        }
        return nearest;
    }

    void MusicSelect::normalizeDifficulty() noexcept {
        const auto* music = selectedMusic();
        if (!music || music->difficulties.isEmpty()) {
            return;
        }
        m_difficulty_code = difficultyFor(*music)->index;
    }
}
