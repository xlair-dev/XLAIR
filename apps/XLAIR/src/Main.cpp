#include "Common.hpp"

#include "app/Application.hpp"
#include "app/flows/Boot.hpp"
#include "infra/api/Factories.hpp"
#include "infra/api/LocalCatalogSync.hpp"
#include "infra/card/Factories.hpp"
#include "infra/config/Loader.hpp"
#include "infra/config/Writer.hpp"
#include "infra/controller/Factories.hpp"
#include "infra/filesystem/RuntimePaths.hpp"
#include "infra/sheets/MetadataLoader.hpp"
#include "ui/Scene.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/AudioBus.hpp"
#include "ui/localization/Localization.hpp"

void Main() {
    // namespace core = xlair::core;
    namespace app = xlair::app;
    namespace infra = xlair::infra;
    namespace ui = xlair::ui;

    ui::audio::InitializeBuses();
    ui::assets::Initialize();
    ui::localization::Initialize();

    const auto paths = infra::filesystem::ResolveRuntimePaths();
    auto application = std::make_shared<app::Application>();
    const auto metadata_loader_factory = [local_directory = paths.sheets_directory](const FilePathView sync_directory) {
        return std::make_unique<infra::sheets::MetadataLoader>(local_directory, FilePath{ sync_directory });
    };
    auto boot_flow = std::make_shared<app::flows::Boot>(
        *application,
        std::make_unique<infra::config::Loader>(paths.config_file, paths.sheets_directory),
        metadata_loader_factory,
        infra::api::CreateClient,
        infra::card::CreateReader,
        infra::controller::CreateDevice,
        infra::api::SyncCatalog
    );
    auto maintenance_flow = std::make_shared<app::flows::Maintenance>(
        *application,
        metadata_loader_factory,
        infra::api::SyncCatalog,
        std::make_unique<infra::config::Writer>(paths.config_file, paths.sheets_directory)
    );
    auto scene_manager = ui::CreateSceneManager(application, boot_flow, maintenance_flow);
    const auto scene_data = scene_manager.get();

    while (System::Update()) {
        application->update(!ui::IsMaintenanceScene(scene_data->current_scene));
        if (!ui::HandleMaintenanceShortcut(scene_manager)) {
            break;
        }
        if (!scene_manager.updateScene()) {
            break;
        }

        scene_data->game_loader.update();
        if (const auto& context = scene_data->music_select_context) {
            context->update(Scene::DeltaTime());
        }
        scene_manager.drawScene();
    }
}
