#include "Common.hpp"

#include "app/Application.hpp"
#include "app/flows/Boot.hpp"
#include "infra/api/Factories.hpp"
#include "infra/card/Factories.hpp"
#include "infra/config/Loader.hpp"
#include "infra/controller/Factories.hpp"
#include "infra/filesystem/RuntimePaths.hpp"
#include "infra/sheets/MetadataLoader.hpp"
#include "ui/Scene.hpp"
#include "ui/assets/Assets.hpp"

void Main() {
    // namespace core = xlair::core;
    namespace app = xlair::app;
    namespace infra = xlair::infra;
    namespace ui = xlair::ui;

    ui::assets::Initialize();

    const auto paths = infra::filesystem::ResolveRuntimePaths();
    auto application = std::make_shared<app::Application>();
    auto boot_flow = std::make_shared<app::flows::Boot>(
        *application,
        std::make_unique<infra::config::Loader>(paths.config_file),
        std::make_unique<infra::sheets::MetadataLoader>(paths.sheets_directory),
        infra::api::CreateClient,
        infra::card::CreateReader,
        infra::controller::CreateDevice,
        infra::api::LocalCatalogSyncFactory{ paths.sheets_directory }
    );
    auto scene_manager = ui::CreateSceneManager(application, boot_flow);

    while (System::Update()) {
        application->update();
        if (!scene_manager.update()) {
            break;
        }
    }
}
