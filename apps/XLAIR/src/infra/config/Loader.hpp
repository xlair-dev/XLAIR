#pragma once

#include "app/interfaces/IConfigLoader.hpp"

namespace xlair::infra::config {
    class Loader final : public app::interfaces::IConfigLoader {
    public:
        explicit Loader(FilePath path);

        [[nodiscard]]
        app::interfaces::ConfigLoadResult load() const override;

    private:
        FilePath m_path;
    };
}
