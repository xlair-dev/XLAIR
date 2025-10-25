#include "Game.hpp"

#include "app/consts/Scene.hpp"
#include "app/usecases/Assets.hpp"
#include "ui/components/PlayerNameplate.hpp"
#include "ui/components/GameMusicPlate.hpp"
#include "ui/components/GameScoreBar.hpp"
#include "ui/addons/HomographyAddon.hpp"
#include "ui/theme/Palette.hpp"

namespace ui {
    Game::Game(const InitData& init) : IScene(init) {}
    Game::~Game() {}

    void Game::update() {
        m_tile_offset_raw += Scene::DeltaTime();
        m_tile_offset = Math::Max(0.0, m_tile_offset_raw);
    }

    void Game::draw() const {
        auto& data = getData();

        drawField();

        //components::DrawPlayerNameplate(getData().playerData, Point{ 59, 72 });
        //components::DrawGameMusicPlate(Point{ 1500, 72 }, data.sheetRepository.getMetadata(data.playerData.selected_index), data.sheetRepository.getJacket(data.playerData.selected_index).value(), data.playerData.selected_difficulty, m_tile_offset, 1);
        //components::DrawGameScoreBar(Point{ app::consts::SceneWidth / 2 - 434, 56 }, data.score);
    }

    void Game::drawField() const {
        constexpr Quad field{
            Vec2{ 850, 0 },
            Vec2{ app::consts::SceneWidth - 850, 0},
            Vec2{ app::consts::SceneWidth - 225, app::consts::SceneHeight},
            Vec2{ 225, app::consts::SceneHeight },
        };

        {
            const auto size = m_rt_main_field.size();
            const auto [w, h] = size;
            const ScopedRenderTarget2D target{ m_rt_main_field.clear(ColorF{ 1.0 }) };

            RectF{ size }.draw(ColorF{ U"#212830" });

            RectF{ 0, 0, 32, h }.draw(ColorF{ U"#B4E6FF" });
            RectF{ w - 32, 0, 32, h }.draw(ColorF{ U"#B4E6FF" });

            //Line{ w / 2, 0, w / 2, h }.draw(3, theme::Palette::White);

            Line{ w / 4.0, 0.0, w / 4.0, h }.draw(2.5, ColorF{ U"#FDFDFD" });
            Line{ w / 2.0, 0.0, w / 2.0, h }.draw(2.5, ColorF{ U"#FDFDFD" });
            Line{ w / 4.0 * 3, 0.0, w / 4.0 * 3, h }.draw(2.5, ColorF{ U"#FDFDFD" });

            FontAsset(app::assets::font::UiComboNumber)(U"279").drawAt(w / 2.0, h - 720, theme::Palette::White);
            FontAsset(app::assets::font::UiComboLabel)(U"COMBO").drawAt(w / 2.0, h - 550, theme::Palette::White);
        }

        Graphics2D::Flush();
        m_rt_main_field.resolve();

        addons::HomographyAddon::Draw(field, m_rt_main_field);

        // debug draw
        RectF{ 1400, 0, 1000 / 4.0, 1000 }(m_rt_main_field).draw();
    }

    void Game::RegisterAssets() {
    }
}
