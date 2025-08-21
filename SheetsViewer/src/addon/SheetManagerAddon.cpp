#include "SheetManagerAddon.hpp"
#include "SUSAnalyzer/SUSAnalyzer.hpp"

#include "addon/NotificationAddon.hpp"
#include "addon/LoadingAnimationAddon.hpp"

SheetManagerAddon::~SheetManagerAddon() {
    if (m_loading_metadata_task.isValid()) {
        m_loading_metadata_task.wait();
    }
}

bool SheetManagerAddon::update() {
    switch (m_state) {
        case State::LoadingMetadata: {
            if (m_loading_metadata_task.isReady()) {
                if (const auto result = m_loading_metadata_task.get()) {
                    m_metadata = *result;
                    m_state = State::LoadingAssets;

                    NotificationAddon::Show(U"Loaded metadata", NotificationAddon::Type::Success);

                    bool failed = false;

                    if (not AudioAsset::Register(MusicAssetName, m_metadata.music)) {
                        NotificationAddon::Show(U"Failed to load music", NotificationAddon::Type::Error);
                        failed = true;
                    }

                    if (not TextureAsset::Register(JacketAssetName, m_metadata.jacket)) {
                        NotificationAddon::Show(U"Failed to load jacket", NotificationAddon::Type::Error);
                        failed = true;
                    }

                    if (failed) {
                        m_metadata = SheetsAnalyzer::Metadata {};
                        m_state = State::Unselected;
                        LoadingAnimationAddon::End();
                    } else {
                        AudioAsset::LoadAsync(MusicAssetName);
                        TextureAsset::LoadAsync(JacketAssetName);
                    }
                } else {
                    NotificationAddon::Show(U"Failed to load file", NotificationAddon::Type::Error);

                    m_metadata = SheetsAnalyzer::Metadata {};
                    m_state = State::Unselected;
                }
            }
        } break;
        case State::LoadingAssets: {
            if (AudioAsset::IsReady(MusicAssetName) && TextureAsset::IsReady(JacketAssetName)) {
                NotificationAddon::Show(U"Loaded assets", NotificationAddon::Type::Success);
                m_state = State::LoadingData;

                const auto sample_rate = AudioAsset(MusicAssetName).sampleRate();
                for (const auto [index, difficulty] : Indexed(m_metadata.difficulties)) {
                    if (difficulty.src.isEmpty()) {
                        m_loading_data_tasks[index] = AsyncTask<Optional<SheetsAnalyzer::SheetData>>([]() {
                            return Optional<SheetsAnalyzer::SheetData> { none };
                        });
                    } else {
                        const auto offset = m_metadata.music_offset;
                        m_loading_data_tasks[index] = AsyncTask<Optional<SheetsAnalyzer::SheetData>>([difficulty, sample_rate, offset]() {
                            return SheetsAnalyzer::SUSAnalyzer::Analyze(difficulty.src, sample_rate, offset);
                        });
                    }
                }
            }
        } break;
        case State::LoadingData: {
            bool all_loaded = true;
            for (const auto& task : m_loading_data_tasks) {
                if (not task.isReady()) {
                    all_loaded = false;
                    break;
                }
            }
            if (all_loaded) {
                for (const auto& [index, difficulty] : Indexed(m_metadata.difficulties)) {
                    auto& task = m_loading_data_tasks[index];

                    if (difficulty.src.isEmpty()) {
                        m_sheet_data[index] = SheetsAnalyzer::SheetData {};
                        continue;
                    }

                    if (const auto result = task.get()) {
                        m_sheet_data[index] = *result;
                    } else {
                        m_sheet_data[index] = SheetsAnalyzer::SheetData {};
                        NotificationAddon::Show(U"Failed to load difficulty: {}"_fmt(index), NotificationAddon::Type::Warning);
                    }
                }

                m_state = State::Loaded;
                NotificationAddon::Show(U"Loaded data", NotificationAddon::Type::Success);
                LoadingAnimationAddon::End();
            }
        } break;
    }
    return true;
}

void SheetManagerAddon::loadAsync(const FilePath& path) {
    m_state = State::LoadingMetadata;
    m_selected_difficulty = none;
    m_loading_metadata_task = AsyncTask<Optional<SheetsAnalyzer::Metadata>>([path]() {
        return SheetsAnalyzer::Analyze(path);
    });

    if (AudioAsset::IsRegistered(MusicAssetName)) {
        AudioAsset(MusicAssetName).stop();
        AudioAsset::Unregister(MusicAssetName);
    }
    if (TextureAsset::IsRegistered(JacketAssetName)) {
        TextureAsset::Unregister(JacketAssetName);
    }

    LoadingAnimationAddon::Begin(Circle { Scene::Center(), 80}, 10, ColorF {0.8, 0.9, 1.0});
}

void SheetManagerAddon::play() const {
    if (IsLoaded()) {
        AudioAsset(MusicAssetName).play();
    }
}

void SheetManagerAddon::pause() const {
    if (isPlaying()) {
        AudioAsset(MusicAssetName).pause();
    }
}

void SheetManagerAddon::stop() const {
    AudioAsset(MusicAssetName).stop();
}

bool SheetManagerAddon::isPlaying() const {
    return AudioAsset(MusicAssetName).isPlaying();
}

double SheetManagerAddon::posSec() const {
    return AudioAsset(MusicAssetName).posSec();
}

double SheetManagerAddon::lengthSec() const {
    return AudioAsset(MusicAssetName).lengthSec();
}

int64 SheetManagerAddon::posSample() const {
    return AudioAsset(MusicAssetName).posSample();
}

void SheetManagerAddon::seekTime(double time) const {
    AudioAsset(MusicAssetName).seekTime(time);
}
