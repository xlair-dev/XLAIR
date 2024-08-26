#include "Common.hpp"
#include "App/usecases/App.hpp"
#include "app/usecases/InitializeApp.hpp"

void Main() {

    app::App app;
    app::InitializeApp(app);

    while (System::Update() and app.sceneController.update()) {
    }
}
