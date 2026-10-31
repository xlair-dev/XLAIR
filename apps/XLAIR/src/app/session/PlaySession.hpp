#pragma once

#include "Common.hpp"
#include "core/judgement/JudgeEngine.hpp"

#include <ApiClient/Types.hpp>

namespace xlair::app::session {
    struct PlayResult {
        UUIDValue play_id;
        String sheet_id;
        uint32 score = 0;
        api::ClearType clear_type = api::ClearType::Failed;
        // Autoplay still consumes a track, but never updates bests or the API.
        bool autoplay = false;
    };

    class PlaySession {
    public:
        void start(api::User user, uint32 max_plays);
        void reset();

        [[nodiscard]]
        bool active() const noexcept;

        [[nodiscard]]
        api::User* user() noexcept;

        [[nodiscard]]
        const api::User* user() const noexcept;

        [[nodiscard]]
        uint32 maxPlays() const noexcept;

        [[nodiscard]]
        uint32 remainingPlays() const noexcept;

        [[nodiscard]]
        bool finished() const noexcept;

        // Reserve a play without consuming it. Only its ID can complete or cancel it.
        [[nodiscard]]
        Optional<UUIDValue> beginPlay(bool autoplay = false);

        [[nodiscard]]
        bool completePlay(
            const UUIDValue& play_id,
            StringView sheet_id,
            const core::judgement::ScoreState& score,
            std::size_t total_combo
        );

        void cancelPlay(const UUIDValue& play_id) noexcept;

        [[nodiscard]]
        const Array<PlayResult>& playedResults() const noexcept;

        void setRecords(Array<api::UserRecord> records);
        void setRatingCatalog(const Array<api::Music>& catalog);

        void setPlayOptions(api::PlayOptions options) noexcept;

        [[nodiscard]]
        api::PlayOptions& playOptions() noexcept;

        [[nodiscard]]
        const api::PlayOptions& playOptions() const noexcept;

        [[nodiscard]]
        bool mirror() const noexcept;

        void setMirror(bool mirror) noexcept;

        [[nodiscard]]
        const api::UserRecord* record(StringView sheet_id) const noexcept;

    private:
        struct ActivePlay {
            UUIDValue id;
            bool autoplay = false;
        };

        struct RatingSheet {
            double level = 0.0;
            bool is_test = false;
        };

        void updateRating();

        Optional<api::User> m_user;
        HashTable<String, api::UserRecord> m_records;
        HashTable<String, RatingSheet> m_rating_sheets;
        Array<PlayResult> m_played_results;
        Optional<ActivePlay> m_active_play;
        api::PlayOptions m_play_options;
        // TODO: Fetch mirror from the API once PlayOptions includes it.
        bool m_mirror = false;
        uint32 m_max_plays = 0;
        uint32 m_remaining_plays = 0;
    };
}
