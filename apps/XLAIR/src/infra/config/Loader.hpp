#pragma once

#include "app/interfaces/IConfigLoader.hpp"

namespace xlair::infra::config {
    class Loader final : public app::interfaces::IConfigLoader {
    public:
        Loader(FilePath path, FilePath local_sheets_directory);

        [[nodiscard]]
        app::interfaces::ConfigLoadResult load() const override;

    private:
        FilePath m_path;
        FilePath m_local_sheets_directory;
    };
}
