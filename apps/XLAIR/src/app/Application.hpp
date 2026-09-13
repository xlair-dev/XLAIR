#pragma once

#include "Common.hpp"

namespace xlair::app {
    class Application {
    public:
        explicit Application(FilePath data_directory);

        [[nodiscard]]
        const FilePath& dataDirectory() const noexcept;

    private:
        FilePath m_data_directory;
    };
}
