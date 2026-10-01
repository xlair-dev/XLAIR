#pragma once

#include "Common.hpp"

#include <ApiClient/Types.hpp>

namespace xlair::app::session {
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

        void consumePlay() noexcept;

        void setRecords(Array<api::UserRecord> records);

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
        Optional<api::User> m_user;
        HashTable<String, api::UserRecord> m_records;
        api::PlayOptions m_play_options;
        // TODO: Fetch mirror from the API once PlayOptions includes it.
        bool m_mirror = false;
        uint32 m_max_plays = 0;
        uint32 m_remaining_plays = 0;
    };
}
