#pragma once
#include "Common.hpp"
#include "app/interfaces/ISheetProvider.hpp" // TODO: fix (core -> app)
#include "core/types/Sheet.hpp"

namespace core::features {
    class SheetRepository : public IAddon {
    private:
        using Provider = std::shared_ptr<app::interfaces::ISheetProvider>;
        using Metadata = core::types::SheetMetadata;
        using Data = core::types::SheetData;

    public:
        SheetRepository(Provider provider);
        ~SheetRepository();
        bool init() override;
        bool update() override;
        bool load();
        bool loadAsync();
        size_t size() const;

        bool loadData(size_t index, uint8 difficulty_index);
        bool loadDataAsync(size_t index, uint8 difficulty_index);
        void releaseData();

        bool isRepoReady() const { return m_state_repo == StateRepo::Ready; }
        bool isDataReady() const { return m_state_data == StateData::Ready; }
        Optional<TextureRegion> getJacket(size_t index) const;
        const Metadata& getMetadata(size_t index) const;
        const Data& getData() const;

        static inline constexpr StringView Name{ U"SheetRepository" };
        static inline constexpr AssetNameView AudioAssetName{ U"SheetRepository.Audio" };

    private:
        static void LoadMetadata(SheetRepository* pSheetRepository);
        static void LoadData(SheetRepository* pSheetRepository, size_t index, uint8 difficulty_index, int64 sample_rate);
        static AssetName GetJacketAssetName(size_t index);

        bool loadJackets(bool do_async = false);
        bool loadAudio(size_t index, bool do_async = false); // TODO

        Provider m_provider;
        AsyncTask<void> m_task_metadata;
        AsyncTask<void> m_task_data;
        bool m_metadata_loaded = false;
        mutable std::mutex m_mutex;
        Array<Metadata> m_metadata;
        Data m_data;

        size_t m_pending_index;
        uint8 m_pending_difficulty_index;

        enum class StateRepo {
            Unloaded,
            LoadingMetadata,
            LoadingJackets,
            Ready,
        };

        enum class StateData {
            Unloaded,
            LoadingAudio,
            LoadingData,
            Ready,
            Failed,
        };

        StateRepo m_state_repo = StateRepo::Unloaded;
        StateData m_state_data = StateData::Unloaded;
    };
}
