# pragma once

# include <Siv3D.hpp>

namespace xlair
{
    [[nodiscard]] inline FilePath DataDirectory()
    {
# if SIV3D_PLATFORM(MACOS)
        // XLAIR.app/Contents/MacOS/XLAIR -> the directory beside XLAIR.app
        const FilePath applicationDirectory = FileSystem::ParentPath(FileSystem::ModulePath(), 3);
# else
        const FilePath applicationDirectory = FileSystem::ParentPath(FileSystem::ModulePath());
# endif

        return FileSystem::PathAppend(applicationDirectory, U"data/");
    }
}
