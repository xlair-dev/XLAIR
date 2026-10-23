#include "PlaySession.hpp"

#include "core/scoring/ClearType.hpp"

#include <utility>

namespace xlair::app::session {
    namespace {
        [[nodiscard]]
        api::ClearType ToApiClearType(const core::scoring::ClearType clear_type) {
            switch (clear_type) {
                case core::scoring::ClearType::Failed:
                    return api::ClearType::Failed;
                case core::scoring::ClearType::Clear:
                    return api::ClearType::Clear;
                case core::scoring::ClearType::FullCombo:
                    return api::ClearType::FullCombo;
                case core::scoring::ClearType::Perfect:
                    return api::ClearType::Perfect;
            }
            return api::ClearType::Failed;
        }

        [[nodiscard]]
        int ClearRank(const api::ClearType clear_type) {
            switch (clear_type) {
                case api::ClearType::Failed:
                    return 0;
                case api::ClearType::Clear:
                    return 1;
                case api::ClearType::FullCombo:
                    return 2;
                case api::ClearType::Perfect:
                    return 3;
            }
            return 0;
        }
    }

    void PlaySession::start(api::User user, const uint32 max_plays) {
        reset();
        m_user = std::move(user);
        m_max_plays = max_plays;
        m_remaining_plays = max_plays;
    }

    void PlaySession::reset() {
        m_user.reset();
        m_records.clear();
        m_played_records.clear();
        m_active_play.reset();
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

    bool PlaySession::finished() const noexcept {
        return active() && m_remaining_plays == 0;
    }

    Optional<UUIDValue> PlaySession::beginPlay() {
        if (!active() || m_remaining_plays == 0 || m_active_play) {
            return none;
        }
        m_active_play = UUIDValue::Generate();
        return m_active_play;
    }

    bool PlaySession::completePlay(
        const UUIDValue& play_id,
        const StringView sheet_id,
        const core::judgement::ScoreState& score,
        const std::size_t total_combo
    ) {
        if (!active() || m_remaining_plays == 0 || !m_active_play || *m_active_play != play_id || sheet_id.isEmpty()) {
            return false;
        }

        const api::RecordSubmission record{
            .sheet_id = String{ sheet_id },
            .score = score.score,
            .clear_type = ToApiClearType(core::scoring::ClearTypeForScore(score, total_combo)),
        };
        m_played_records.push_back({ .play_id = play_id, .record = record });

        auto& best = m_records[record.sheet_id];
        best.sheet_id = record.sheet_id;
        best.score = Max(best.score, record.score);
        if (ClearRank(record.clear_type) > ClearRank(best.clear_type)) {
            best.clear_type = record.clear_type;
        }

        --m_remaining_plays;
        m_active_play.reset();
        return true;
    }

    void PlaySession::cancelPlay(const UUIDValue& play_id) noexcept {
        if (m_active_play && *m_active_play == play_id) {
            m_active_play.reset();
        }
    }

    const Array<PlayedRecord>& PlaySession::playedRecords() const noexcept {
        return m_played_records;
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
