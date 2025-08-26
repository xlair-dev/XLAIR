#pragma once
#include "addon/SheetManagerAddon.hpp"

inline SheetManagerAddon::State SheetManagerAddon::GetState() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        return p->m_state;
    }
    return State::Unselected;
}

inline void SheetManagerAddon::LoadAsync(const FilePath& path) {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        p->loadAsync(path);
    }
}

inline bool SheetManagerAddon::IsLoading() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        return p->m_state == State::LoadingMetadata or p->m_state == State::LoadingAssets or p->m_state == State::LoadingData;
    }
    return false;
}

inline bool SheetManagerAddon::IsLoaded() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        return p->m_state == State::Loaded;
    }
    return false;
}

inline const SheetsAnalyzer::Metadata& SheetManagerAddon::GetMetadata() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        return p->m_metadata;
    }
    return SheetsAnalyzer::Metadata {};
}

inline Optional<SheetsAnalyzer::SheetData> SheetManagerAddon::GetSheetData(size_t index) {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        if (index < SheetsAnalyzer::Constant::MaxDifficulties) {
            return p->m_sheet_data[index];
        }
    }
    return none;
}

inline Optional<TextureRegion> SheetManagerAddon::GetJacket() {
    if (IsLoaded()) {
        if (TextureAsset(JacketAssetName)) {
            return TextureRegion { TextureAsset(JacketAssetName) };
        } else {
            return none;
        }
    }
    return none;
}

inline void SheetManagerAddon::Play() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        p->play();
    }
}

inline void SheetManagerAddon::Pause() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        p->pause();
    }
}

inline void SheetManagerAddon::Stop() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        p->stop();
    }
}

inline void SheetManagerAddon::Toggle() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        if (p->isPlaying()) {
            p->pause();
        } else {
            p->play();
        }
    }
}

inline bool SheetManagerAddon::IsPlaying() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        return p->isPlaying();
    }
    return false;
}

inline double SheetManagerAddon::PosSec() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        return p->posSec();
    }
    return 0.0;
}

inline double SheetManagerAddon::LengthSec() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        return p->lengthSec();
    }
    return 0.0;
}

inline int64 SheetManagerAddon::PosSample() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        return p->posSample();
    }
    return 0;
}

inline void SheetManagerAddon::SeekTime(double time) {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        p->seekTime(time);
    }
}

inline void SheetManagerAddon::SetSelectedDifficulty(Optional<size_t> index) {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        if (const auto value = index; value and *value < SheetsAnalyzer::Constant::MaxDifficulties) {
            p->m_selected_difficulty = *value;
            return;
        } else {
            p->m_selected_difficulty = none;
        }
    }
}

inline Optional<size_t> SheetManagerAddon::GetSelectedDifficulty() {
    if (auto p = Addon::GetAddon<SheetManagerAddon>(Name)) {
        return p->m_selected_difficulty;
    }
    return none;
}
