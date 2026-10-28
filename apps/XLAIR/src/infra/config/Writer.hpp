#pragma once

#include "app/interfaces/IConfigWriter.hpp"

namespace xlair::infra::config {
    class Writer final : public app::interfaces::IConfigWriter {
    public:
        Writer(FilePath path, FilePath local_sheets_directory);

        [[nodiscard]]
        Optional<app::interfaces::ConfigSaveError>
        save(const app::Config& saved, const app::Config& current) const override;

    private:
        FilePath m_path;
        FilePath m_local_sheets_directory;
    };
}
