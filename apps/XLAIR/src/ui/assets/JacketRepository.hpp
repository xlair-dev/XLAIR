#pragma once

#include "Common.hpp"

#include <SheetsAnalyzer/Metadata.hpp>
#include <SheetsAnalyzer/Result.hpp>

namespace xlair::ui::assets {
    class JacketRepository {
    public:
        enum class State {
            Idle,
            Loading,
            Ready,
        };

        JacketRepository();
        ~JacketRepository();

        JacketRepository(const JacketRepository&) = delete;
        JacketRepository& operator=(const JacketRepository&) = delete;

        void start(const Array<sheets::Metadata>& metadata);
        void update();
        void clear();

        [[nodiscard]]
        Texture get(StringView music_id) const;

        [[nodiscard]]
        State state() const noexcept;

        [[nodiscard]]
        std::size_t completedCount() const noexcept;

        [[nodiscard]]
        std::size_t totalCount() const noexcept;

        [[nodiscard]]
        std::size_t fallbackCount() const noexcept;

        [[nodiscard]]
        const Array<sheets::Diagnostic>& diagnostics() const noexcept;

    private:
        enum class EntryState {
            Pending,
            Loading,
            Loaded,
            Fallback,
        };

        struct Entry {
            String music_id;
            FilePath path;
            AssetName asset_name;
            EntryState state = EntryState::Pending;
        };

        static constexpr std::size_t MaxConcurrentLoads = 4;

        [[nodiscard]]
        static AssetName MakeAssetName(StringView music_id);

        void startPendingLoads();
        void addWarning(String message, FilePath path);

        Texture m_fallback;
        Array<Entry> m_entries;
        HashTable<String, std::size_t> m_indices;
        Array<sheets::Diagnostic> m_diagnostics;
        std::size_t m_completed_count = 0;
        std::size_t m_fallback_count = 0;
        std::size_t m_active_count = 0;
        std::size_t m_next_load_index = 0;
        State m_state = State::Idle;
    };
}
