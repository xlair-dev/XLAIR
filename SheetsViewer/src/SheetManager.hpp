#pragma once
#include <Siv3D.hpp>

#include "SheetsAnalyzer.hpp"
#include "SUSAnalyzer/SUSAnalyzer.hpp"

class SheetManager {
public:

    enum class State {
        Unselected,
        LoadingMetadata,
        LoadingAssets,
        LoadingData,
        Loaded,
    };

    SheetManager(const String& base);
    ~SheetManager();

    void loadAsync(const FilePath& path);

    void update();

    void play();

    void pause();

    void stop();

    bool isPlaying() const;

    double posSec() const;

    double lengthSec() const;

    void seekTime(double time);

    inline State getState() const noexcept {
        return m_state;
    }

    inline bool isLoading() const noexcept {
        return m_state == State::LoadingMetadata or m_state == State::LoadingAssets or m_state == State::LoadingData;
    }

    inline bool isLoaded() const noexcept {
        return m_state == State::Loaded;
    }

    inline const SheetsAnalyzer::Metadata& getMetadata() const noexcept {
        return m_metadata;
    }

    inline const std::array<SheetsAnalyzer::SheetData, SheetsAnalyzer::Constant::MaxDifficulties>& getSheetData() const noexcept {
        return m_sheet_data;
    }

    inline const SheetsAnalyzer::SheetData& getSheetData(size_t index) const {
        if (index < SheetsAnalyzer::Constant::MaxDifficulties) {
            return m_sheet_data[index];
        }
        throw std::out_of_range("Index out of range");
    }

    inline Optional<TextureRegion> getJacket() const {
        if (isLoaded()) {
            if (TextureAsset(JacketAssetName)) {
                return TextureRegion { TextureAsset(JacketAssetName) };
            } else {
                return none;
            }
        }
        return none;
    }

private:
    const AssetName MusicAssetName;
    const AssetName JacketAssetName;

    AsyncTask<Optional<SheetsAnalyzer::Metadata>> m_loading_metadata_task;
    std::array<AsyncTask<Optional<SheetsAnalyzer::SheetData>>, SheetsAnalyzer::Constant::MaxDifficulties> m_loading_data_tasks;

    SheetsAnalyzer::Metadata m_metadata;

    State m_state = State::Unselected;
    std::array<SheetsAnalyzer::SheetData, SheetsAnalyzer::Constant::MaxDifficulties> m_sheet_data;
};
