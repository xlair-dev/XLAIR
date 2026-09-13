#include "Common.hpp"

#include "app/Application.hpp"
#include "infra/api/CreateClient.hpp"
#include "infra/config/Loader.hpp"
#include "infra/filesystem/RuntimePaths.hpp"
#include "ui/Scene.hpp"

void Main() {
    // namespace core = xlair::core;
    namespace app = xlair::app;
    namespace infra = xlair::infra;
    namespace ui = xlair::ui;

    const auto paths = infra::filesystem::ResolveRuntimePaths();
    auto config_loader = std::make_unique<infra::config::Loader>(paths.config_file);
    auto application = std::make_shared<app::Application>(std::move(config_loader), infra::api::CreateClient);
    auto scene_manager = ui::CreateSceneManager(application);

    while (System::Update() && scene_manager.update()) {
    }
}
