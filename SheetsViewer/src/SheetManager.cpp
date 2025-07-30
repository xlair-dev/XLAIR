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
            NotificationAddon::Show(U"Loaded", NotificationAddon::Type::Success);
            m_state = State::Loaded;
            LoadingAnimationAddon::End();
        }
    }
}
