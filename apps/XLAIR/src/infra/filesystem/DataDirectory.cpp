#include "infra/filesystem/DataDirectory.hpp"

namespace xlair::infra::filesystem {
    FilePath DataDirectory() {
#if SIV3D_PLATFORM(MACOS)
        // XLAIR.app/Contents/MacOS/XLAIR -> the directory beside XLAIR.app
        const FilePath application_directory = FileSystem::ParentPath(FileSystem::ModulePath(), 3);
#else
        const FilePath application_directory = FileSystem::ParentPath(FileSystem::ModulePath());
#endif

        return FileSystem::PathAppend(application_directory, U"data/");
    }
}
