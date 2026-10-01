#include "Common.hpp"

#include "app/Application.hpp"
#include "app/flows/Boot.hpp"
#include "infra/api/Factories.hpp"
#include "infra/api/LocalCatalogSync.hpp"
#include "infra/card/Factories.hpp"
#include "infra/config/Loader.hpp"
#include "infra/controller/Factories.hpp"
#include "infra/filesystem/RuntimePaths.hpp"
#include "infra/sheets/MetadataLoader.hpp"
#include "ui/Scene.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/localization/Localization.hpp"

void Main() {
    // namespace core = xlair::core;
    namespace app = xlair::app;
    namespace infra = xlair::infra;
    namespace ui = xlair::ui;

    ui::assets::Initialize();
    ui::localization::Initialize();

    const auto paths = infra::filesystem::ResolveRuntimePaths();
    auto application = std::make_shared<app::Application>();
    auto boot_flow = std::make_shared<app::flows::Boot>(
        *application,
        std::make_unique<infra::config::Loader>(paths.config_file, paths.sheets_directory),
        [local_directory = paths.sheets_directory](const FilePathView sync_directory) {
            return std::make_unique<infra::sheets::MetadataLoader>(local_directory, FilePath{ sync_directory });
        },
        infra::api::CreateClient,
        infra::card::CreateReader,
        infra::controller::CreateDevice,
        infra::api::SyncCatalog
    );
    auto scene_manager = ui::CreateSceneManager(application, boot_flow);
    const auto scene_data = scene_manager.get();

    while (System::Update()) {
        application->update();
        if (!scene_manager.updateScene()) {
            break;
        }

        if (const auto& context = scene_data->music_select_context) {
            context->update(Scene::DeltaTime());
        }
        scene_manager.drawScene();
    }
}
