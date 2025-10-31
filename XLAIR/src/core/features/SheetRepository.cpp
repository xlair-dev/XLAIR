#include "SheetRepository.hpp"

namespace core::features {
    SheetRepository::SheetRepository(Provider provider)
        : m_provider(std::move(provider)) {
    }

    SheetRepository::~SheetRepository() {
        if (m_task_metadata.isValid()) {
            m_task_metadata.wait();
        }
        if (m_task_data.isValid()) {
            m_task_data.wait();
        }
    }

    bool SheetRepository::init() {
        return true;
    }

    bool SheetRepository::update() {
        switch (m_state_repo) {
            case StateRepo::Unloaded: break;
            case StateRepo::LoadingMetadata: {
                if (m_task_metadata.isReady()) {
                    m_state_repo = StateRepo::LoadingJackets;
                    loadJackets(true);
                }
            } break;
            case StateRepo::LoadingJackets: {
                bool all_loaded = true;
                for (size_t i = 0 ; i < m_metadata.size(); ++i) {
                    std::scoped_lock lock(m_mutex);
                    const auto name = GetJacketAssetName(i);
                    if (not TextureAsset::IsReady(name)) {
                        all_loaded = false;
                        break;
                    }
                }
                if (all_loaded) {
                    m_state_repo = StateRepo::Ready;
                }
            } break;
            case StateRepo::Ready: break;
            default: break;
        }

        switch (m_state_data) {
            case StateData::Unloaded: break;
            case StateData::LoadingAudio: {
                if (AudioAsset::IsReady(AudioAssetName)) {
                    m_state_data = StateData::LoadingData;
                    m_task_data = Async([this]() {
                        LoadData(this, m_pending_index, m_pending_difficulty_index, AudioAsset(AudioAssetName).sampleRate());
                    });
                }
            } break;
            case StateData::LoadingData: {
                if (m_task_data.isReady()) {
                    m_state_data = StateData::Ready;
                }
            } break;
            case StateData::Ready: break;
            default: break;
        }

        return true;
    }

    bool SheetRepository::load() {
        if (m_state_repo != StateRepo::Unloaded or !m_provider) {
            return false;
        }
        LoadMetadata(this);
        loadJackets();
        m_state_repo = StateRepo::Ready;
        return true;
    }

    bool SheetRepository::loadAsync() {
        if (m_state_repo != StateRepo::Unloaded or !m_provider) {
            return false;
        }
        m_state_repo = StateRepo::LoadingMetadata;
        m_metadata_loaded = false;
        m_task_metadata = Async([this]() {
            LoadMetadata(this);
        });
        return true;
    }

    void SheetRepository::releaseData() {
        std::scoped_lock lock(m_mutex);
        m_data = Data{};
        m_state_data = StateData::Unloaded;
        AudioAsset::Release(AudioAssetName);
        AudioAsset::Unregister(AudioAssetName);
    }

    bool SheetRepository::loadData(size_t index, uint8 difficulty_index) {
        if (not m_metadata_loaded or !m_provider or index >= m_metadata.size()) {
            return false;
        }
        m_pending_index = index;
        m_pending_difficulty_index = difficulty_index;
        loadAudio(index);
        LoadData(this,index, difficulty_index, AudioAsset(AudioAssetName).sampleRate());
        m_state_data = StateData::Ready;
        return true;
    }

    bool SheetRepository::loadDataAsync(size_t index, uint8 difficulty_index) {
        if (not m_metadata_loaded or !m_provider or index >= m_metadata.size()) {
            return false;
        }
        loadAudio(index, true);
        m_state_data = StateData::LoadingAudio;
        return true;
    }

    size_t SheetRepository::size() const {
        if (not m_metadata_loaded) {
            return 0;
        }
        std::scoped_lock lock(m_mutex);
        return m_metadata.size();
    }

    Optional<TextureRegion> SheetRepository::getJacket(size_t index) const {
        if (!m_metadata_loaded || index >= m_metadata.size()) {
            return none;
        }
        const auto name = GetJacketAssetName(index);
        if (TextureAsset::IsReady(name)) {
            return TextureRegion{ TextureAsset(name) };
        }
        return none;
    }

    const core::types::SheetMetadata& SheetRepository::getMetadata(size_t index) const {
        std::scoped_lock lock(m_mutex);
        if (not m_metadata_loaded or index >= m_metadata.size()) {
            return {};
        }
        return m_metadata[index];
    }

    const core::types::SheetData& SheetRepository::getData() const {
        std::scoped_lock lock(m_mutex);
        if (m_state_data != StateData::Ready) {
            return {};
        }
        return m_data;
    }

    void SheetRepository::LoadMetadata(SheetRepository* pSheetRepository) {
        pSheetRepository->m_metadata_loaded = false;
        {
            std::scoped_lock lock(pSheetRepository->m_mutex);
            pSheetRepository->m_metadata.clear();
            pSheetRepository->m_provider->load(pSheetRepository->m_metadata);
        }
        pSheetRepository->m_metadata_loaded = true;
    }

    void SheetRepository::LoadData(SheetRepository* pSheetRepository, size_t index, uint8 difficulty_index, int64 sample_rate) {
        std::scoped_lock lock(pSheetRepository->m_mutex);
        pSheetRepository->m_provider->loadData(pSheetRepository->m_data, pSheetRepository->m_metadata[index].difficulties[difficulty_index].src, sample_rate, pSheetRepository->m_metadata[index].music_offset);
    }

    bool SheetRepository::loadJackets(bool do_async) {
        if (not m_metadata_loaded) {
            return false;
        }
        std::scoped_lock lock(m_mutex);

        for (size_t i = 0; i < m_metadata.size(); ++i) {
            const auto name = GetJacketAssetName(i);
            const auto& path = m_metadata[i].jacket;

            if (TextureAsset::IsRegistered(name)) {
                TextureAsset::Unregister(name);
            }
            TextureAsset::Register(name, path, TextureDesc::Mipped); // TODO: handle Error
            if (do_async) {
                TextureAsset::LoadAsync(name);
            } else {
                TextureAsset::Load(name);
            }
        }
        return true;
    }

    bool SheetRepository::loadAudio(size_t index, bool do_async) {
        if (not m_metadata_loaded or index >= m_metadata.size()) {
            return false;
        }

        if (AudioAsset::IsRegistered(AudioAssetName)) {
            AudioAsset::Unregister(AudioAssetName);
        }
        const auto& path = m_metadata[index].music;
        AudioAsset::Register(AudioAssetName, path); // TODO: handle Error
        if (do_async) {
            AudioAsset::LoadAsync(AudioAssetName);
        } else {
            AudioAsset::Load(AudioAssetName);
        }

        return true;
    }



    AssetName SheetRepository::GetJacketAssetName(size_t index) {
        return U"SheetRepositoryJacket_{}"_fmt(index);
    }
}
