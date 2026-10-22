#include <ThirdParty/Catch2/catch.hpp>

#include "app/flows/CardReaderTest.hpp"
#include "app/flows/LightTest.hpp"

namespace app = xlair::app;
namespace controller = app::controller;
using LightTest = app::flows::LightTest;

namespace {
    struct ScanState {
        Optional<app::card::Result> result;
        bool cancelled = false;
    };

    class Scan final : public app::card::IScan {
    public:
        explicit Scan(std::shared_ptr<ScanState> state) : m_state{ std::move(state) } {}
        void update() override {}
        void cancel() override {
            m_state->cancelled = true;
        }
        const Optional<app::card::Result>& result() const noexcept override {
            return m_state->result;
        }

    private:
        std::shared_ptr<ScanState> m_state;
    };

    class Reader final : public app::interfaces::ICardReader {
    public:
        Array<std::shared_ptr<ScanState>> scans;

        app::card::ScanRequest scan() override {
            auto state = std::make_shared<ScanState>();
            scans.push_back(state);
            return std::make_unique<Scan>(std::move(state));
        }
    };

    class Device final : public app::interfaces::IControllerDevice {
    public:
        controller::InputSnapshot input;
        controller::LightFrame output;
        controller::LightCapabilities capabilities{ .slider = true, .side = true };
        bool fail_lights = false;
        std::size_t writes = 0;

        controller::OperationResult initialize() override {
            return {};
        }
        controller::OperationResult update() override {
            return {};
        }
        controller::OperationResult setLights(const controller::LightFrame& lights) override {
            ++writes;
            output = lights;
            if (fail_lights) {
                return controller::OperationResult::makeError(controller::ErrorKind::Communication, U"Write failed.");
            }
            return {};
        }
        const controller::InputSnapshot& snapshot() const noexcept override {
            return input;
        }
        controller::LightCapabilities lightCapabilities() const noexcept override {
            return capabilities;
        }
    };

    struct ControllerFixture {
        Device* device = nullptr;
        controller::Controller device_controller;
        controller::LightFrame original;

        ControllerFixture() : device_controller{ makeDevice() } {
            REQUIRE(device_controller.initialize());
            original.slider.fill(Palette::Purple);
            original.side.fill(Palette::Green);
            REQUIRE(device_controller.setLights(original));
        }

    private:
        std::unique_ptr<Device> makeDevice() {
            auto result = std::make_unique<Device>();
            device = result.get();
            return result;
        }
    };
}

TEST_CASE("Card reader tests read repeatedly and cancel pending scans", "[Maintenance][Card]") {
    Reader reader;
    app::flows::CardReaderTest test;
    test.start(&reader);
    REQUIRE(test.waiting());
    reader.scans.back()->result = app::card::Card{ U"00000001" };
    test.update();
    REQUIRE_FALSE(test.waiting());
    REQUIRE(test.result().has_value());
    CHECK(std::get<app::card::Card>(*test.result()).card_id == U"00000001");

    test.start(&reader);
    REQUIRE(reader.scans.size() == 2);
    CHECK_FALSE(test.result().has_value());
    test.start(&reader);
    CHECK(reader.scans[1]->cancelled);
    test.stop();
    CHECK(reader.scans.back()->cancelled);
    CHECK_FALSE(test.waiting());
}

TEST_CASE("Card reader tests display reader failures and stop on destruction", "[Maintenance][Card]") {
    Reader reader;
    {
        app::flows::CardReaderTest test;
        test.start(nullptr);
        REQUIRE(test.result().has_value());
        CHECK(std::get<app::card::Error>(*test.result()).kind == app::card::ErrorKind::Unavailable);

        test.start(&reader);
        reader.scans.back()->result = app::card::Error{ app::card::ErrorKind::Communication, U"Read failed." };
        test.update();
        REQUIRE(test.result().has_value());
        CHECK(std::get<app::card::Error>(*test.result()).message == U"Read failed.");
        test.start(&reader);
    }
    CHECK(reader.scans.back()->cancelled);
}

TEST_CASE("LED tests preserve the other output and restore the original lights", "[Maintenance][Lights]") {
    ControllerFixture fixture;
    {
        LightTest test{ fixture.device_controller };
        REQUIRE(test.supported());
        test.start(LightTest::Pattern::Red);
        for (const auto color : fixture.device->output.slider) {
            CHECK(color == Palette::Red);
        }
        CHECK(fixture.device->output.side == fixture.original.side);
        const auto writes = fixture.device->writes;
        test.update(0.1);
        CHECK(fixture.device->writes == writes);
    }
    CHECK(fixture.device->output == fixture.original);
    CHECK(fixture.device_controller.lights() == fixture.original);
}

TEST_CASE("LED target changes stop the previous output and preserve its original state", "[Maintenance][Lights]") {
    ControllerFixture fixture;
    LightTest test{ fixture.device_controller };
    test.start(LightTest::Pattern::Red);
    test.changeTarget();
    CHECK(test.target() == LightTest::Target::Side);
    CHECK_FALSE(test.activePattern().has_value());
    CHECK(fixture.device->output == fixture.original);
    test.start(LightTest::Pattern::Blue);
    CHECK(fixture.device->output.slider == fixture.original.slider);
    for (const auto color : fixture.device->output.side) {
        CHECK(color == Palette::Blue);
    }
}

TEST_CASE("LED tests advance the sequence and follow controller input", "[Maintenance][Lights]") {
    ControllerFixture fixture;
    LightTest test{ fixture.device_controller };
    test.start(LightTest::Pattern::Sequence);
    CHECK(fixture.device->output.slider[0] == Palette::White);
    test.update(0.25);
    CHECK(fixture.device->output.slider[0] == Palette::Black);
    CHECK(fixture.device->output.slider[2] == Palette::White);

    fixture.device->input.touch_zones[4] = true;
    REQUIRE(fixture.device_controller.update());
    test.start(LightTest::Pattern::Input);
    for (std::size_t index = 0; index < controller::SliderLightCount; ++index) {
        CHECK(fixture.device->output.slider[index] == (index >= 3 && index <= 5 ? Palette::White : Palette::Black));
    }
    test.changeTarget();
    fixture.device->input.side_buttons[static_cast<std::size_t>(controller::SideButton::RightUpper)] = true;
    REQUIRE(fixture.device_controller.update());
    test.start(LightTest::Pattern::Input);
    CHECK(fixture.device->output.side[static_cast<std::size_t>(controller::SideButton::RightUpper)] == Palette::White);
    CHECK(fixture.device->output.side[static_cast<std::size_t>(controller::SideButton::LeftUpper)] == Palette::Black);
}

TEST_CASE("LED tests reject unsupported hardware without writing", "[Maintenance][Lights]") {
    ControllerFixture fixture;
    fixture.device->capabilities = {};
    LightTest test{ fixture.device_controller };
    REQUIRE_FALSE(test.supported());
    const auto writes = fixture.device->writes;
    test.start(LightTest::Pattern::Red);
    CHECK(test.error().has_value());
    CHECK_FALSE(test.activePattern().has_value());
    CHECK(fixture.device->writes == writes);
}

TEST_CASE("LED tests restore after a failed write and never select unsupported outputs", "[Maintenance][Lights]") {
    ControllerFixture fixture;
    fixture.device->capabilities.side = false;
    LightTest test{ fixture.device_controller };
    test.changeTarget();
    CHECK(test.target() == LightTest::Target::Slider);
    fixture.device->fail_lights = true;
    test.start(LightTest::Pattern::Red);
    REQUIRE(test.error().has_value());
    CHECK_FALSE(test.activePattern().has_value());
    fixture.device->fail_lights = false;
    test.stop();
    CHECK(fixture.device->output == fixture.original);
}

TEST_CASE("LED tests resend a previous color after a partial write failure", "[Maintenance][Lights]") {
    ControllerFixture fixture;
    LightTest test{ fixture.device_controller };
    test.start(LightTest::Pattern::Red);
    fixture.device->fail_lights = true;
    test.start(LightTest::Pattern::Blue);
    REQUIRE(test.error().has_value());
    fixture.device->fail_lights = false;
    test.start(LightTest::Pattern::Red);
    CHECK_FALSE(test.error().has_value());
    for (const auto color : fixture.device->output.slider) {
        CHECK(color == Palette::Red);
    }
}
