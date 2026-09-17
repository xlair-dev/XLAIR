#include "Title.hpp"

namespace xlair::ui::scenes {
    Title::Title(const InitData& init) : SceneBase{ init } {
        startScan();
    }

    Title::~Title() {
        if (m_scan) {
            m_scan->cancel();
        }
    }

    void Title::update() {
        if (!m_scan) {
            if (KeyR.down()) {
                ClearPrint();
                startScan();
            }
            return;
        }

        m_scan->update();
        const auto& result = m_scan->result();
        if (!result) {
            return;
        }

        if (const auto* card = std::get_if<app::card::Card>(&*result)) {
            Print << U"Card ID: " + card->card_id;
        } else {
            const auto& error = std::get<app::card::Error>(*result);
            if (error.kind != app::card::ErrorKind::Cancelled) {
                Print << U"Card reader error: " + error.message;
            }
        }
        Print << U"Press R to scan again.";
        m_scan.reset();
    }

    void Title::draw() const {
        m_font(U"XLAIR").draw(40, 40, Palette::White);
    }

    void Title::startScan() {
        auto* reader = getData().application->cardReader();
        if (!reader) {
            Print << U"Card reader is not available.";
            Print << U"Press R to retry.";
            return;
        }

        m_scan = reader->scan();
        if (!m_scan) {
            Print << U"Failed to start card scan.";
            Print << U"Press R to retry.";
            return;
        }

        Print << U"Waiting for a card...";
        if (getData().application->config()->card_reader.mode == app::Config::CardReader::Mode::Mock) {
            Print << U"Press Space to scan the mock card.";
        }
    }
}
