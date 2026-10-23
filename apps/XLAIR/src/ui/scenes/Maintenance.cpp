#include "Maintenance.hpp"

#include "ui/assets/Assets.hpp"
#include "ui/audio/AudioBus.hpp"

#include <array>
#include <cmath>
#include <span>

namespace xlair::ui::scenes {
    namespace {
        constexpr std::array<StringView, 5> MenuItems{
            U"コントローラテスト", U"カードリーダーテスト", U"サーバ通信", U"サウンド", U"Title に戻る",
        };
        constexpr std::array<StringView, 3> ControllerItems{ U"入力テスト", U"LED テスト", U"戻る" };
        constexpr std::array<StringView, 7> LightItems{
            U"消灯", U"赤", U"緑", U"青", U"白", U"順番に点灯", U"入力に連動して点灯",
        };
        constexpr std::array LightPatterns{
            app::flows::LightTest::Pattern::Off,   app::flows::LightTest::Pattern::Red,
            app::flows::LightTest::Pattern::Green, app::flows::LightTest::Pattern::Blue,
            app::flows::LightTest::Pattern::White, app::flows::LightTest::Pattern::Sequence,
            app::flows::LightTest::Pattern::Input,
        };
        constexpr std::array<StringView, 4> ServerItems{ U"API ヘルスチェック",
                                                         U"同期する",
                                                         U"全件再検証・修復",
                                                         U"戻る" };
        constexpr std::array<StringView, 4> SoundItems{ U"楽曲", U"UI SE", U"判定・ホールド SE", U"戻る" };
        constexpr std::array SoundBuses{ audio::Music, audio::UISound, audio::GameplaySound };
        constexpr double SoundRepeatDelay = 0.4;
        constexpr double SoundRepeatInterval = 0.1;
        constexpr double SoundFastRepeatAfter = 1.5;
        constexpr double SoundFastRepeatInterval = 0.03;

        void DrawMenu(
            const std::span<const StringView> items,
            const std::size_t selected,
            const Optional<std::size_t> disabled = none
        ) {
            const FontAsset font{ assets::font::Text };
            for (std::size_t index = 0; index < items.size(); ++index) {
                const double y = 128.0 + index * 52.0;
                if (index == selected) {
                    font(U"▶").draw(28, Vec2{ 48, y }, Palette::White);
                }
                const bool enabled = !disabled || *disabled != index;
                font(items[index]).draw(28, Vec2{ 92, y }, enabled ? Palette::White : Palette::Gray);
                if (!enabled) {
                    font(U"（未対応）").draw(24, Vec2{ 290, y + 3 }, Palette::Gray);
                }
            }
        }

        void DrawButton(const RectF& rect, const StringView label, const app::controller::ButtonState& state) {
            rect.draw(state.pressed() ? Palette::Red : Palette::Black).drawFrame(1, Palette::White);
            FontAsset{ assets::font::Text }(label).drawAt(24, rect.center().movedBy(0, -16), Palette::White);
            FontAsset{
                assets::font::Text
            }(!state.available() ? U"--"
              : state.pressed()  ? U"ON"
                                 : U"OFF")
                .drawAt(24, rect.center().movedBy(0, 18), Palette::White);
        }
    }

    Maintenance::Maintenance(const InitData& init) : SceneBase{ init } {
        auto& data = getData();
        if (const auto& config = data.application->config()) {
            m_highlight_touch_threshold = config->controller.highlight_touch_threshold;
        }
        if (data.music_select_context) {
            data.music_select_context->pause();
        }
        data.game_loader.cancel();
        data.maintenance_flow->reset();
        ClearPrint();
    }

    Maintenance::~Maintenance() {
        m_card_reader_test.stop();
        m_light_test.reset();
        getData().maintenance_flow->reset();
    }

    bool Maintenance::busy() const {
        return getData().maintenance_flow->busy() || m_loading_jackets;
    }

    void Maintenance::setPage(const Page page) {
        m_card_reader_test.stop();
        m_light_test.reset();
        m_page = page;
        m_selected_index = 0;
        resetSoundRepeat();
        if (page == Page::Lights) {
            if (auto* controller = getData().application->controller()) {
                m_light_test = std::make_unique<app::flows::LightTest>(*controller);
            }
        } else if (page == Page::CardReader) {
            m_card_reader_test.start(getData().application->cardReader());
        }
    }

    std::span<const StringView> Maintenance::menuItems() const {
        switch (m_page) {
            case Page::Menu:
                return MenuItems;
            case Page::Controller:
                return ControllerItems;
            case Page::Lights:
                return LightItems;
            case Page::Server:
                return ServerItems;
            case Page::Sound:
                return SoundItems;
            default:
                return {};
        }
    }

    bool Maintenance::itemEnabled(const std::size_t index) const {
        if (m_page == Page::Controller && index == 1) {
            const auto* controller = getData().application->controller();
            if (!controller) {
                return false;
            }
            const auto capabilities = controller->lightCapabilities();
            return capabilities.slider || capabilities.side;
        }
        return true;
    }

    void Maintenance::update() {
        // The opening press is still down when a zero-duration transition updates the new scene.
        if (Scene::FrameCount() == m_opened_frame) {
            return;
        }
        updateServer();
        if (busy()) {
            return;
        }
        const auto* controller = getData().application->controller();
        if (controller && controller->maintenanceButton(app::controller::MaintenanceButton::Button1).down()) {
            changeScene(SceneState::Title, 0);
            return;
        }
        if (KeyLeft.down() && m_page != Page::Menu && m_page != Page::Sound) {
            back();
            return;
        }
        if (m_page == Page::Slider) {
            updateSlider();
            return;
        }
        if (m_page == Page::CardReader) {
            updateCardReader();
            return;
        }
        if (m_page == Page::Lights) {
            if (!m_light_test || !m_light_test->supported()) {
                return;
            }
            if (KeyRight.down()) {
                m_light_test->changeTarget();
            }
            m_light_test->update(Scene::DeltaTime());
        }
        const auto items = menuItems();
        const int32 direction = (KeyDown.down() ? 1 : 0) - (KeyUp.down() ? 1 : 0);
        if (direction != 0 && !items.empty()) {
            do {
                m_selected_index = (m_selected_index + items.size() + direction) % items.size();
            } while (!itemEnabled(m_selected_index));
            resetSoundRepeat();
        }
        if (m_page == Page::Sound) {
            updateSound();
            return;
        }
        if (!KeyEnter.down() || !itemEnabled(m_selected_index)) {
            return;
        }
        if (m_page == Page::Menu) {
            switch (m_selected_index) {
                case 0:
                    setPage(Page::Controller);
                    break;
                case 1:
                    setPage(Page::CardReader);
                    break;
                case 2:
                    setPage(Page::Server);
                    break;
                case 3:
                    setPage(Page::Sound);
                    break;
                default:
                    changeScene(SceneState::Title, 0);
                    break;
            }
        } else if (m_page == Page::Controller) {
            if (m_selected_index == 2) {
                back();
            } else {
                setPage(m_selected_index == 0 ? Page::Slider : Page::Lights);
            }
        } else if (m_page == Page::Lights) {
            m_light_test->start(LightPatterns[m_selected_index]);
        } else if (m_selected_index == 0) {
            getData().maintenance_flow->checkHealth();
        } else if (m_selected_index == 1 || m_selected_index == 2) {
            // Discard the selection before replacing the catalog it references.
            getData().music_select_context.reset();
            getData().maintenance_flow->sync(
                m_selected_index == 2 ? app::flows::CatalogSync::Mode::VerifyAll
                                      : app::flows::CatalogSync::Mode::Incremental
            );
            m_sync_requested = true;
        } else {
            back();
        }
    }

    void Maintenance::back() {
        const auto previous = m_page;
        if (previous == Page::Slider || previous == Page::Lights) {
            setPage(Page::Controller);
            m_selected_index = previous == Page::Lights ? 1 : 0;
        } else {
            setPage(Page::Menu);
            m_selected_index = previous == Page::Sound        ? 3
                               : previous == Page::Server     ? 2
                               : previous == Page::CardReader ? 1
                                                              : 0;
        }
    }

    void Maintenance::resetSoundRepeat() {
        m_sound_repeat_direction = 0;
        m_sound_hold_seconds = 0.0;
        m_sound_next_repeat_seconds = SoundRepeatDelay;
    }

    void Maintenance::updateSound() {
        if (m_selected_index == SoundBuses.size()) {
            resetSoundRepeat();
            if (KeyEnter.down()) {
                back();
            }
            return;
        }
        const int32 direction = (KeyRight.pressed() ? 1 : 0) - (KeyLeft.pressed() ? 1 : 0);
        if (direction == 0) {
            resetSoundRepeat();
            return;
        }

        int32 steps = 0;
        if (direction != m_sound_repeat_direction) {
            resetSoundRepeat();
            m_sound_repeat_direction = direction;
            steps = 1;
        } else {
            m_sound_hold_seconds += Scene::DeltaTime();
            // Catch up across slow frames, but never apply more than the full volume range.
            while (m_sound_hold_seconds >= m_sound_next_repeat_seconds && steps < 100) {
                ++steps;
                m_sound_next_repeat_seconds +=
                    m_sound_next_repeat_seconds >= SoundFastRepeatAfter ? SoundFastRepeatInterval : SoundRepeatInterval;
            }
            if (steps == 100) {
                m_sound_next_repeat_seconds = m_sound_hold_seconds + SoundFastRepeatInterval;
            }
        }
        if (steps == 0) {
            return;
        }
        const auto bus = SoundBuses[m_selected_index];
        const int32 volume = static_cast<int32>(std::lround(GlobalAudio::BusGetVolume(bus) * 100.0));
        GlobalAudio::BusSetVolume(bus, Clamp(volume + direction * steps, 0, 100) / 100.0);
    }

    void Maintenance::updateSlider() {
        if (KeyEnter.down()) {
            m_highlight_touch_threshold = !m_highlight_touch_threshold;
        }
        auto* controller = getData().application->controller();
        if (!controller) {
            return;
        }
        if (const auto threshold = controller->touchThreshold()) {
            const int32 change = (KeyUp.down() ? 1 : 0) - (KeyDown.down() ? 1 : 0);
            if (change != 0) {
                controller->setTouchThreshold(
                    static_cast<uint8>(Clamp(static_cast<int32>(*threshold) + change, 1, 255))
                );
            }
        }
    }

    void Maintenance::updateCardReader() {
        if (KeyEnter.down()) {
            m_card_reader_test.start(getData().application->cardReader());
        }
        m_card_reader_test.update();
    }

    void Maintenance::updateServer() {
        auto& data = getData();
        auto& flow = *data.maintenance_flow;
        flow.update();
        if (m_sync_requested && flow.state() == app::flows::Maintenance::State::Succeeded) {
            data.jackets.start(data.application->musicCatalog());
            m_loading_jackets = true;
            m_sync_requested = false;
        } else if (flow.state() == app::flows::Maintenance::State::Failed) {
            m_sync_requested = false;
        }
        if (m_loading_jackets) {
            data.jackets.update();
            m_loading_jackets = data.jackets.state() != assets::JacketAssets::State::Ready;
        }
    }

    void Maintenance::draw() const {
        Scene::Rect().draw(Palette::Black);
        const FontAsset font{ assets::font::Text };
        const StringView title = m_page == Page::Controller   ? U"コントローラテスト"
                                 : m_page == Page::Slider     ? U"入力テスト"
                                 : m_page == Page::Lights     ? U"LED テスト"
                                 : m_page == Page::CardReader ? U"カードリーダーテスト"
                                 : m_page == Page::Server     ? U"サーバ通信"
                                 : m_page == Page::Sound      ? U"サウンド"
                                                              : U"MAINTENANCE";
        font(title).draw(36, Vec2{ 48, 40 }, Palette::White);
        if (m_page == Page::Slider) {
            drawSlider();
        } else if (m_page == Page::Lights) {
            drawLights();
        } else if (m_page == Page::CardReader) {
            drawCardReader();
        } else if (m_page == Page::Server) {
            drawServer();
        } else if (m_page == Page::Sound) {
            drawSound();
        } else {
            const Optional<std::size_t> disabled =
                m_page == Page::Controller && !itemEnabled(1) ? Optional<std::size_t>{ 1 } : none;
            DrawMenu(menuItems(), m_selected_index, disabled);
        }
        const StringView guide =
            busy()                       ? U"処理中..."
            : m_page == Page::Slider     ? U"↑ / ↓: 閾値    Enter: 緑表示切替    ←: 戻る    F2: Title"
            : m_page == Page::Lights     ? U"↑ / ↓: 選択    Enter: 点灯    →: 出力先切替    ←: 戻る    F2: Title"
            : m_page == Page::CardReader ? U"Enter: 再読取    ←: 戻る    F2: Title"
            : m_page == Page::Sound ? U"↑ / ↓: 選択    ← / →: 音量（長押し可・起動中のみ）    Enter: 決定    F2: Title"
                                    : U"↑ / ↓: 選択    Enter: 決定    ←: 戻る    F2: Title";
        font(guide).draw(20, Vec2{ 48, Scene::Height() - 56.0 }, Palette::White);
    }

    void Maintenance::drawSound() const {
        DrawMenu(SoundItems, m_selected_index);
        const FontAsset font{ assets::font::Text };
        for (std::size_t index = 0; index < SoundBuses.size(); ++index) {
            const auto volume = static_cast<int32>(std::lround(GlobalAudio::BusGetVolume(SoundBuses[index]) * 100.0));
            font(U"{}%"_fmt(volume)).draw(28, Vec2{ 480, 128.0 + index * 52.0 }, Palette::White);
        }
    }

    void Maintenance::drawSlider() const {
        const auto& application = *getData().application;
        const auto* controller = getData().application->controller();
        const FontAsset font{ assets::font::Text };
        if (!controller) {
            font(U"Controller is not available.").draw(28, Vec2{ 48, 128 }, Palette::White);
            return;
        }
        const auto threshold = controller->touchThreshold();
        font(threshold ? U"閾値: {}"_fmt(static_cast<int32>(*threshold)) : U"閾値: -- (Keyboard)")
            .draw(28, Vec2{ 48, 128 }, Palette::White);
        font(U"しきい値超過を緑表示: {}"_fmt(m_highlight_touch_threshold ? U"ON" : U"OFF"))
            .draw(24, Vec2{ 48, 168 }, Palette::White);

        constexpr double Left = 48;
        constexpr double Top = 210;
        const double width = (Scene::Width() - Left * 2) / 16.0;
        for (std::size_t index = 0; index < app::controller::TouchZoneCount; ++index) {
            const RectF cell{ Left + (index / 2) * width, Top + (index % 2) * 120.0, width, 120 };
            const auto value = controller->touchValue(index);
            const auto& button = controller->touchZone(index);
            const double red = value ? Min(1.0, *value / 254.0) : button.pressed() ? 1.0 : 0.0;
            const bool above_threshold = m_highlight_touch_threshold && threshold && value && *value >= *threshold;
            cell.draw(above_threshold ? ColorF{ 0, 1, 0 } : ColorF{ red, 0, 0 }).drawFrame(1, Palette::White);
            font(index).drawAt(18, cell.center().movedBy(0, -30), Palette::White);
            font(value ? Format(static_cast<int32>(*value)) : U"--")
                .drawAt(30, cell.center().movedBy(0, 8), Palette::White);
        }

        constexpr std::array<StringView, 4> SideLabels{ U"左上", U"左下", U"右上", U"右下" };
        for (std::size_t index = 0; index < SideLabels.size(); ++index) {
            DrawButton(
                RectF{ Left + index * 180.0, 500, 160, 86 },
                SideLabels[index],
                controller->sideButton(static_cast<app::controller::SideButton>(index))
            );
            DrawButton(
                RectF{ Left + index * 180.0, 620, 160, 86 },
                U"メンテ {}"_fmt(index + 1),
                controller->maintenanceButton(static_cast<app::controller::MaintenanceButton>(index))
            );
        }
        if (const auto& error = application.controllerError()) {
            font(error->message).draw(24, RectF{ 48, 760, Scene::Width() - 96.0, 200 }, Palette::White);
        }
    }

    void Maintenance::drawLights() const {
        DrawMenu(LightItems, m_selected_index);
        const FontAsset font{ assets::font::Text };
        if (!m_light_test || !m_light_test->supported()) {
            font(U"このコントローラは LED 出力に対応していません。").draw(26, Vec2{ 530, 128 }, Palette::White);
            return;
        }
        const auto target = m_light_test->target();
        font(target == app::flows::LightTest::Target::Slider ? U"出力: スライダー" : U"出力: サイド")
            .draw(28, Vec2{ 530, 128 }, Palette::White);
        if (const auto active = m_light_test->activePattern()) {
            font(U"実行中: {}"_fmt(LightItems[static_cast<std::size_t>(*active)]))
                .draw(26, Vec2{ 530, 190 }, Palette::White);
        }
        if (const auto& error = m_light_test->error()) {
            font(error->message).draw(26, RectF{ 530, 270, Scene::Width() - 578.0, 300 }, Palette::White);
        }
    }

    void Maintenance::drawCardReader() const {
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

    void Maintenance::drawServer() const {
        DrawMenu(ServerItems, m_selected_index);
        const auto& flow = *getData().maintenance_flow;
        const FontAsset font{ assets::font::Text };
        if (const auto& config = getData().application->config()) {
            font(config->api.syncSource()).draw(22, RectF{ 48, 340, Scene::Width() - 96.0, 60 }, Palette::White);
        }
        const String message = m_loading_jackets ? U"Loading jackets..." : flow.message();
        font(message).draw(26, RectF{ 48, 420, Scene::Width() - 96.0, 400 }, Palette::White);
    }
}
