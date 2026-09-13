#include "Common.hpp"
#include "app/Application.hpp"
#include "infra/filesystem/DataDirectory.hpp"
#include "ui/RootView.hpp"

void Main() {
    xlair::app::Application application{ xlair::infra::filesystem::DataDirectory() };
    const xlair::ui::RootView root_view;

    while (System::Update()) {
        root_view.draw(application);
    }
}
