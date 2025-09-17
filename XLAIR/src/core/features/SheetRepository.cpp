#include "SheetRepository.hpp"

namespace core::features {
    SheetRepository::~SheetRepository() {
        if (m_loading_task.isValid()) {
            m_loading_task.wait();
        }
    }

    bool SheetRepository::load(Provider provider) {
        if (m_loading_task.isValid()) {
            m_loading_task.wait();
        }
        Load(this, std::move(provider));
        return true;
    }

    bool SheetRepository::loadAsync(Provider provider) {
        if (m_loading_task.isValid()) {
            return false;
        }
        m_loading_task = Async([this, p = std::move(provider)]() mutable {
            Load(this, std::move(p));
        });
        return true;
    }

    bool SheetRepository::loadJacket() {
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
            TextureAsset::Load(name);
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
            return TextureRegion { TextureAsset(name) };
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

    void SheetRepository::Load(SheetRepository* pSheetRepository, Provider provider) {
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
