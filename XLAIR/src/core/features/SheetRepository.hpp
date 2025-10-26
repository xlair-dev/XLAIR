#pragma once
#include "Common.hpp"
#include "app/interfaces/ISheetProvider.hpp"
#include "core/types/Sheet.hpp"

namespace core::features {
    class SheetRepository : public IAddon {
    private:
        using Provider = std::unique_ptr<app::interfaces::ISheetProvider>;

    public:
        ~SheetRepository();
        bool init() override;
        bool update() override;
        bool load(Provider provider);
        bool loadAsync(Provider provider);
        size_t size() const;

        //// bool loadData(size_t index, Provider provider); // TODO: implement
        //// bool loadDataAsync(size_t index, Provider provider); // TODO: implement

        Optional<TextureRegion> getJacket(size_t index) const;
        core::types::SheetMetadata getMetadata(size_t index) const;

        static inline constexpr StringView Name{ U"SheetRepository" };

    private:
        static void LoadMetadata(SheetRepository* pSheetRepository, Provider provider);
        static AssetName GetJacketAssetName(size_t index);

        bool loadJackets(bool do_async = false);

        AsyncTask<void> m_loading_task;
        std::atomic<bool> m_metadata_loaded{ false };
        mutable std::mutex m_mutex;

        Array<core::types::SheetMetadata> m_metadata;
        core::types::SheetData m_data;

        enum class State {
            Unloaded,
            LoadingMetadata,
            LoadingJackets,
            Loaded,
        };

        State m_state = State::Unloaded;
    };
}
