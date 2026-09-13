#pragma once

#include "Common.hpp"

namespace xlair::infra::filesystem {
    struct RuntimePaths {
        FilePath application_directory;
        FilePath config_file;
        FilePath data_directory;
        FilePath sheets_directory;
    };

    [[nodiscard]]
    RuntimePaths ResolveRuntimePaths();
}
