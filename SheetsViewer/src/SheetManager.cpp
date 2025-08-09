#include "SheetManager.hpp"
#include "addon/NotificationAddon.hpp"
#include "addon/LoadingAnimationAddon.hpp"

SheetManager::SheetManager(const String& base)
    : MusicAssetName(base + U"_MusicAsset")
    , JacketAssetName(base + U"_JacketAsset") {
}

SheetManager::~SheetManager() {
    if (m_loading_metadata_task.isValid()) {
        m_loading_metadata_task.wait();
    }
}

void SheetManager::loadAsync(const FilePath& path) {
    m_state = State::LoadingMetadata;
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

void SheetManager::update() {
    if (m_state == State::LoadingMetadata) {
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
    } else if (m_state == State::LoadingAssets) {
        if (AudioAsset::IsReady(MusicAssetName) && TextureAsset::IsReady(JacketAssetName)) {
            NotificationAddon::Show(U"Loaded assets", NotificationAddon::Type::Success);
            m_state = State::LoadingData;

            for (const auto [index, difficulty] : Indexed(m_metadata.difficulties)) {
                if (difficulty.src.isEmpty()) {
                    m_loading_data_tasks[index] = AsyncTask<Optional<SheetsAnalyzer::SheetData>>([]() {
                        return Optional<SheetsAnalyzer::SheetData> { none };
                    });
                } else {
                    m_loading_data_tasks[index] = AsyncTask<Optional<SheetsAnalyzer::SheetData>>([difficulty]() {
                        return SheetsAnalyzer::SUSAnalyzer::Analyze(difficulty.src);
                    });
                }
            }
        }
    } else if (m_state == State::LoadingData) {
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
    }
}

void SheetManager::play() const {
    if (isLoaded()) {
        AudioAsset(MusicAssetName).play();
    }
}

void SheetManager::pause() const {
    if (isPlaying()) {
        AudioAsset(MusicAssetName).pause();
    }
}

void SheetManager::stop() const {
    AudioAsset(MusicAssetName).stop();
}

bool SheetManager::isPlaying() const {
    return AudioAsset(MusicAssetName).isPlaying();
}

double SheetManager::posSec() const {
    return AudioAsset(MusicAssetName).posSec();
}

double SheetManager::lengthSec() const {
    return AudioAsset(MusicAssetName).lengthSec();
}

void SheetManager::seekTime(double time) const {
    if (isLoaded()) {
        AudioAsset(MusicAssetName).seekTime(time);
    }
}
