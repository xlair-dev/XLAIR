#include "JacketRepository.hpp"

namespace xlair::ui::assets {
    JacketRepository::JacketRepository() : m_fallback{ Image{ 8, 8, Color{ 32, 36, 48 } }, TextureDesc::Mipped } {}

    JacketRepository::~JacketRepository() {
        clear();
    }

    void JacketRepository::start(const Array<sheets::Metadata>& metadata) {
        clear();
        m_entries.reserve(metadata.size());
        m_indices.reserve(metadata.size());

        for (const auto& music : metadata) {
            Entry entry{
                .music_id = music.id,
                .path = music.jacket,
                .asset_name = MakeAssetName(music.id),
            };
            const std::size_t index = m_entries.size();
            m_indices.emplace(entry.music_id, index);

            if (entry.path.isEmpty()) {
                entry.state = EntryState::Fallback;
                ++m_completed_count;
                ++m_fallback_count;
            }
            m_entries.push_back(std::move(entry));
        }

        m_state = m_completed_count == m_entries.size() ? State::Ready : State::Loading;
        startPendingLoads();
    }

    void JacketRepository::update() {
        if (m_state != State::Loading) {
            return;
        }

        for (auto& entry : m_entries) {
            if (entry.state != EntryState::Loading || !TextureAsset::IsReady(entry.asset_name)) {
                continue;
            }

            if (TextureAsset{ entry.asset_name }) {
                entry.state = EntryState::Loaded;
            } else {
                addWarning(U"Failed to load the jacket image.", entry.path);
                TextureAsset::Unregister(entry.asset_name);
                entry.state = EntryState::Fallback;
                ++m_fallback_count;
            }
            --m_active_count;
            ++m_completed_count;
        }

        startPendingLoads();
        if (m_completed_count == m_entries.size()) {
            m_state = State::Ready;
        }
    }

    void JacketRepository::clear() {
        for (const auto& entry : m_entries) {
            if ((entry.state != EntryState::Loading && entry.state != EntryState::Loaded) ||
                !TextureAsset::IsRegistered(entry.asset_name)) {
                continue;
            }
            if (entry.state == EntryState::Loading) {
                TextureAsset::Wait(entry.asset_name);
            }
            TextureAsset::Unregister(entry.asset_name);
        }
        m_entries.clear();
        m_indices.clear();
        m_diagnostics.clear();
        m_completed_count = 0;
        m_fallback_count = 0;
        m_active_count = 0;
        m_next_load_index = 0;
        m_state = State::Idle;
    }

    Texture JacketRepository::get(const StringView music_id) const {
        if (const auto iterator = m_indices.find(music_id); iterator != m_indices.end()) {
            const auto& entry = m_entries[iterator->second];
            if (entry.state == EntryState::Loaded && TextureAsset::IsRegistered(entry.asset_name)) {
                return TextureAsset{ entry.asset_name };
            }
        }
        return m_fallback;
    }

    JacketRepository::State JacketRepository::state() const noexcept {
        return m_state;
    }

    std::size_t JacketRepository::completedCount() const noexcept {
        return m_completed_count;
    }

    std::size_t JacketRepository::totalCount() const noexcept {
        return m_entries.size();
    }

    std::size_t JacketRepository::fallbackCount() const noexcept {
        return m_fallback_count;
    }

    const Array<sheets::Diagnostic>& JacketRepository::diagnostics() const noexcept {
        return m_diagnostics;
    }

    AssetName JacketRepository::MakeAssetName(const StringView music_id) {
        return AssetName{ U"XLAIR.Jacket." + music_id };
    }

    void JacketRepository::startPendingLoads() {
        while (m_active_count < MaxConcurrentLoads && m_next_load_index < m_entries.size()) {
            auto& entry = m_entries[m_next_load_index++];
            if (entry.state != EntryState::Pending) {
                continue;
            }

            if (!TextureAsset::Register(entry.asset_name, entry.path, TextureDesc::Mipped)) {
                addWarning(U"Failed to register the jacket image.", entry.path);
                entry.state = EntryState::Fallback;
                ++m_completed_count;
                ++m_fallback_count;
                continue;
            }

            TextureAsset::LoadAsync(entry.asset_name);
            entry.state = EntryState::Loading;
            ++m_active_count;
        }
    }

    void JacketRepository::addWarning(String message, FilePath path) {
        m_diagnostics.push_back(
            {
                .severity = sheets::DiagnosticSeverity::Warning,
                .message = std::move(message),
                .path = std::move(path),
            }
        );
    }
}
