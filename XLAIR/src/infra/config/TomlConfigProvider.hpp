#pragma once
#include "app/interfaces/IConfigProvider.hpp"

namespace infra::config {
    class TomlConfigProvider : public app::interfaces::IConfigProvider {
    public:
        explicit TomlConfigProvider(FilePath path) : m_path(std::move(path)) {}
        void load(app::types::Config& config) override;

    private:
        FilePath m_path;
    };
}
