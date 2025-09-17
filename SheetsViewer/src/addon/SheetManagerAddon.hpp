#pragma once
#include <Siv3D.hpp>

#include "SheetsAnalyzer.hpp"

class SheetManagerAddon : public IAddon {
public:

    inline static constexpr StringView Name = U"SheetManagerAddon";

    enum class State {
        Unselected,
        LoadingMetadata,
        LoadingAssets,
        LoadingData,
        Loaded,
    };

    ~SheetManagerAddon() override;

    static State GetState();
    static void LoadAsync(const FilePath& path);
    static bool IsLoading();
    static bool IsLoaded();
    static const SheetsAnalyzer::Metadata& GetMetadata();
    static Optional<SheetsAnalyzer::SheetData> GetSheetData(size_t index);
    static Optional<TextureRegion> GetJacket();

    static void Play();
    static void Pause();
    static void Stop();
    static void Toggle();
    static bool IsPlaying();
    static double PosSec();
    static double LengthSec();
    static int64 PosSample();
    static void SeekTime(double time);

    static void SetSelectedDifficulty(Optional<size_t> index);
    static inline Optional<size_t> GetSelectedDifficulty();

private:
    inline static constexpr StringView MusicAssetName = U"SheetManager_MusicAsset";
    inline static constexpr StringView JacketAssetName = U"SheetManager_JacketAsset";

    State m_state = State::Unselected;
    Optional<size_t> m_selected_difficulty = none;

    SheetsAnalyzer::Metadata m_metadata;
    AsyncTask<Optional<SheetsAnalyzer::Metadata>> m_loading_metadata_task;

    std::array<SheetsAnalyzer::SheetData, SheetsAnalyzer::Constant::MaxDifficulties> m_sheet_data;
    std::array<AsyncTask<Optional<SheetsAnalyzer::SheetData>>, SheetsAnalyzer::Constant::MaxDifficulties> m_loading_data_tasks;

    bool update() override;

    void loadAsync(const FilePath& path);

    void play() const;
    void pause() const;
    void stop() const;
    bool isPlaying() const;
    double posSec() const;
    double lengthSec() const;
    int64 posSample() const;
    void seekTime(double time) const;
};

#include "detail/SheetManagerAddon.ipp"
