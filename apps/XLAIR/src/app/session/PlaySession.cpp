#include "PlaySession.hpp"

#include <utility>

namespace xlair::app::session {
    void PlaySession::start(api::User user, const uint32 max_plays) {
        m_user = std::move(user);
        m_records.clear();
        m_play_options = {};
        m_mirror = false;
        m_max_plays = max_plays;
        m_remaining_plays = max_plays;
    }

    void PlaySession::reset() {
        m_user.reset();
        m_records.clear();
        m_play_options = {};
        m_mirror = false;
        m_max_plays = 0;
        m_remaining_plays = 0;
    }

    bool PlaySession::active() const noexcept {
        return m_user.has_value();
    }

    api::User* PlaySession::user() noexcept {
        return m_user ? &*m_user : nullptr;
    }

    const api::User* PlaySession::user() const noexcept {
        return m_user ? &*m_user : nullptr;
    }

    uint32 PlaySession::maxPlays() const noexcept {
        return m_max_plays;
    }

    uint32 PlaySession::remainingPlays() const noexcept {
        return m_remaining_plays;
    }

    void PlaySession::consumePlay() noexcept {
        if (m_remaining_plays > 0) {
            --m_remaining_plays;
        }
    }

    void PlaySession::setRecords(Array<api::UserRecord> records) {
        m_records.clear();
        m_records.reserve(records.size());
        for (auto& record : records) {
            m_records[record.sheet_id] = std::move(record);
        }
    }

    void PlaySession::setPlayOptions(const api::PlayOptions options) noexcept {
        m_play_options = options;
    }

    api::PlayOptions& PlaySession::playOptions() noexcept {
        return m_play_options;
    }

    const api::PlayOptions& PlaySession::playOptions() const noexcept {
        return m_play_options;
    }

    bool PlaySession::mirror() const noexcept {
        return m_mirror;
    }

    void PlaySession::setMirror(const bool mirror) noexcept {
        m_mirror = mirror;
    }

    const api::UserRecord* PlaySession::record(const StringView sheet_id) const noexcept {
        const auto iterator = m_records.find(sheet_id);
        return iterator == m_records.end() ? nullptr : &iterator->second;
    }
}
