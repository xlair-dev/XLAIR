#include "Title.hpp"
#include "core/features/CardReaderManager.hpp"
#include "app/consts/Version.hpp"
#include "app/consts/Scene.hpp"
#include "app/usecases/Assets.hpp"
#include "ui/theme/Palette.hpp"

namespace ui {
    Title::Title(const InitData& init) : IScene(init) {}

    Title::~Title() {}

    void Title::update() {
        using core::features::CardReaderManager;

        if (m_loading) {
            m_call.update();
            if (m_call.isReady()) {
                if (m_call.isOK()) {
                    getData().userData = m_call.value();
                    changeScene(app::types::SceneState::MusicSelect, 0.5s);
                } else {
                    m_loading = false;
                    m_scaned = false;
                    m_card_scanning = false;
                }
            }
            return;
        }

        if (m_scaned) {
            m_call = getData().api->getUserByCard(CardReaderManager::GetID());
            m_loading = true;
            return;
        }

        if (m_card_scanning) {
            if (CardReaderManager::IsReady()) {
                if (CardReaderManager::IsOK()) {
                    m_scaned = true;
                } else {
                    // TODO: error handling
                     CardReaderManager::StartScan();
                }
            }
        } else {
            CardReaderManager::StartScan();
            m_card_scanning = true;
        }
    }

    void Title::draw() const {
        using app::consts::SceneWidth;
        using app::consts::SceneHeight;
        TextureAsset(app::assets::texture::Logo).drawAt(SceneWidth / 2.0, SceneHeight * 0.4);

        const auto region = FontAsset(app::assets::font::UiText)(U"カードをタッチして始める").drawAt(
            32,
            Vec2{ SceneWidth / 2.0,  760 },
            theme::Palette::Cyan
        ).stretched(30);

        RectF{ Arg::rightCenter = region.leftCenter(), 135, 2}.draw(theme::Palette::Cyan);
        RectF{ Arg::leftCenter = region.rightCenter(), 135, 2}.draw(theme::Palette::Cyan);

        FontAsset(app::assets::font::UiText)(U"XLAIR version: " XLAIR_VERSION_STRING).draw(
            15,
            Arg::bottomRight(SceneWidth - 10, SceneHeight - 5),
            theme::Palette::Gray
        );
    }
}
