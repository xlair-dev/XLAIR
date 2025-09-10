#include "MusicSelect.hpp"

#include "app/consts/Scene.hpp"

namespace ui {
    MusicSelect::MusicSelect(const InitData& init) : IScene(init) {
        Print << U"MusicSelect";
    }

    MusicSelect::~MusicSelect() {}

    void MusicSelect::update() {}

    void MusicSelect::draw() const {
        Size TileSize { 416, 505 };

        FontAsset(U"BrunoAce")(U"MUSIC SELECT").drawAt(70, app::consts::SceneWidth / 2, 135);
        FontAsset(U"tile.text")(U"楽曲を選択してください").drawAt(24, app::consts::SceneWidth / 2, 203);

        Line { 397, 130, 574, 130 }.draw(3);
        Line { 397, 147, 574, 147 }.draw(3);

        Line { 1367, 130, 1544, 130 }.draw(3);
        Line { 1367, 147, 1544, 147 }.draw(3);

        // getData().sheetRepository.getJacket(0)->draw();

        // RectF { app::consts::SceneSize.maxComponent() }(*getData().sheetRepository.getJacket(0)).draw(ColorF{ 1, 1, 1, 0.05 });

        RectF{TileSize}(
            m_tile.get(getData().sheetRepository.getMetadata(0), *getData().sheetRepository.getJacket(0), 0)
        ).draw(100, 300);

        RectF { 416, 505 }.draw();
    }
}
