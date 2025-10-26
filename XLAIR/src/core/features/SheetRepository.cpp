#include "SheetRepository.hpp"

namespace core::features {
    SheetRepository::~SheetRepository() {
        if (m_loading_task.isValid()) {
            m_loading_task.wait();
        }
    }

    bool SheetRepository::init() {
        return true;
    }

    bool SheetRepository::update() {
        switch (m_state) {
            case State::Unloaded: break;
            case State::LoadingMetadata: {
                if (m_loading_task.isReady()) {
                    m_state = State::LoadingJackets;
                    loadJackets(true);
                }
            } break;
            case State::LoadingJackets: {
                std::scoped_lock lock(m_mutex);
                bool all_loaded = true;
                for (size_t i = 0 ; i < m_metadata.size(); ++i) {
                    const auto name = GetJacketAssetName(i);
                    if (not TextureAsset::IsReady(name)) {
                        all_loaded = false;
                        break;
                    }
                }
                if (all_loaded) {
                    m_state = State::Loaded;
                }
            } break;
            case State::Loaded: break;
            default: break;
        }
        return true;
    }

    bool SheetRepository::load(Provider provider) {
        if (m_state != State::Unloaded) {
            return false;
        }
        LoadMetadata(this, std::move(provider));
        loadJackets();
        m_state = State::Loaded;
        return true;
    }

    bool SheetRepository::loadAsync(Provider provider) {
        if (m_state != State::Unloaded) {
            return false;
        }
        m_state = State::LoadingMetadata;
        m_loading_task = Async([this, p = std::move(provider)]() mutable {
            LoadMetadata(this, std::move(p));
        });
        return true;
    }

    bool SheetRepository::loadJackets(bool do_async) {
        if (not m_metadata_loaded.load(std::memory_order_acquire)) {
            return false;
        }
        std::scoped_lock lock(m_mutex);
        for (size_t i = 0; i < m_metadata.size(); ++i) {
            const auto name = GetJacketAssetName(i);
            const auto& path = m_metadata[i].jacket;

            if (TextureAsset::IsRegistered(name)) {
                TextureAsset::Unregister(name);
            }
            TextureAsset::Register(name, path, TextureDesc::Mipped);
            if (do_async) {
                TextureAsset::LoadAsync(name);
            } else {
                TextureAsset::Load(name);
            }
        }
        return true;
    }

    size_t SheetRepository::size() const {
        if (not m_metadata_loaded.load(std::memory_order_acquire)) {
            return 0;
        }
        std::scoped_lock lock(m_mutex);
        return m_metadata.size();
    }

    Optional<TextureRegion> SheetRepository::getJacket(size_t index) const {
        const auto name = GetJacketAssetName(index);
        if (TextureAsset::IsReady(name)) {
            return TextureRegion{ TextureAsset(name) };
        }
        return none;
    }

    core::types::SheetMetadata SheetRepository::getMetadata(size_t index) const {
        std::scoped_lock lock(m_mutex);
        if (not m_metadata_loaded.load(std::memory_order_acquire) or index >= m_metadata.size()) {
            return {};
        }
        return m_metadata[index];
    }

    void SheetRepository::LoadMetadata(SheetRepository* pSheetRepository, Provider provider) {
        pSheetRepository->m_metadata_loaded = false;
        {
            std::scoped_lock lock(pSheetRepository->m_mutex);
            pSheetRepository->m_metadata.clear();
            provider->load(pSheetRepository->m_metadata);
        }
        pSheetRepository->m_metadata_loaded = true;
    }

    AssetName SheetRepository::GetJacketAssetName(size_t index) {
        return U"SheetRepositoryJacket_{}"_fmt(index);
    }
}
