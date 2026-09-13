#include "RuntimePaths.hpp"

namespace xlair::infra::filesystem {
    RuntimePaths ResolveRuntimePaths() {
        const FilePath application_directory = FileSystem::ParentPath(FileSystem::ModulePath());
        const FilePath data_directory = FileSystem::PathAppend(application_directory, U"data/");

        return {
            .application_directory = application_directory,
            .config_file = FileSystem::PathAppend(application_directory, U"config.toml"),
            .data_directory = data_directory,
            .sheets_directory = FileSystem::PathAppend(data_directory, U"sheets/"),
        };
    }
}
