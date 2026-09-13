#include "Common.hpp"

#include "app/Application.hpp"
#include "infra/filesystem/DataDirectory.hpp"
#include "ui/Scene.hpp"

void Main() {
    // namespace core = xlair::core;
    namespace app = xlair::app;
    namespace infra = xlair::infra;
    namespace ui = xlair::ui;

    auto application = std::make_shared<app::Application>(infra::filesystem::DataDirectory());
    auto scene_manager = ui::CreateSceneManager(application);

    while (System::Update() && scene_manager.update()) {
    }
}
