#include "Common.hpp"

#include "app/Application.hpp"
#include "app/BootFlow.hpp"
#include "infra/api/Factories.hpp"
#include "infra/config/Loader.hpp"
#include "infra/filesystem/RuntimePaths.hpp"
#include "infra/sheets/MetadataLoader.hpp"
#include "ui/Scene.hpp"

void Main() {
    // namespace core = xlair::core;
    namespace app = xlair::app;
    namespace infra = xlair::infra;
    namespace ui = xlair::ui;

    const auto paths = infra::filesystem::ResolveRuntimePaths();
    auto config_loader = std::make_unique<infra::config::Loader>(paths.config_file);
    auto application = std::make_shared<app::Application>();
    auto boot_flow = std::make_shared<app::BootFlow>(
        *application,
        std::move(config_loader),
        std::make_unique<infra::sheets::MetadataLoader>(paths.sheets_directory),
        infra::api::CreateClient,
        infra::api::LocalCatalogSyncFactory{ paths.sheets_directory }
    );
    auto scene_manager = ui::CreateSceneManager(application, boot_flow);

    while (System::Update() && scene_manager.update()) {
    }
}
