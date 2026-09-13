#include "app/Application.hpp"

#include <utility>

namespace xlair::app {
    Application::Application(FilePath data_directory) : m_data_directory{ std::move(data_directory) } {}

    const FilePath& Application::dataDirectory() const noexcept {
        return m_data_directory;
    }
}
