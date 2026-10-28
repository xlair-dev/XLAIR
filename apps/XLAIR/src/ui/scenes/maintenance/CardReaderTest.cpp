#include "CardReaderTest.hpp"

#include "ui/assets/Assets.hpp"

namespace xlair::ui::scenes::maintenance {
    CardReaderTest::CardReaderTest(const InitData& init) : Base{ init } {
        m_card_reader_test.start(getData().application->cardReader());
    }

    void CardReaderTest::update() {
        if (!navigate(SceneState::Maintenance)) {
            return;
        }
        if (KeyEnter.down()) {
            m_card_reader_test.start(getData().application->cardReader());
        }
        m_card_reader_test.update();
    }

    void CardReaderTest::draw() const {
        drawFrame(U"カードリーダーテスト", U"Enter: 再読取    ←: 戻る    F2: Title");
        const FontAsset font{ assets::font::Text };
        if (m_card_reader_test.waiting()) {
            font(U"カードをかざしてください。").draw(28, Vec2{ 48, 128 }, Palette::White);
        }
        if (const auto& result = m_card_reader_test.result()) {
            if (const auto* card = std::get_if<app::card::Card>(&*result)) {
                font(U"読み取り成功").draw(28, Vec2{ 48, 128 }, Palette::White);
                font(U"card_id: " + card->card_id)
                    .draw(32, RectF{ 48, 200, Scene::Width() - 96.0, 200 }, Palette::White);
            } else if (const auto* error = std::get_if<app::card::Error>(&*result)) {
                font(U"読み取りエラー").draw(28, Vec2{ 48, 128 }, Palette::White);
                font(error->message).draw(26, RectF{ 48, 200, Scene::Width() - 96.0, 200 }, Palette::White);
            }
        }
        if (const auto& config = getData().application->config();
            config && config->card_reader.mode == app::Config::CardReader::Mode::Mock) {
            font(U"Mock: Space でカードを読み取り").draw(24, Vec2{ 48, 430 }, Palette::White);
        }
    }
}
