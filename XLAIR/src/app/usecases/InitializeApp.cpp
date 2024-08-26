#include "InitializeApp.hpp"
#include "core/types/Scene.hpp"
#include "app/di/SceneController.hpp"

namespace app {
    void InitializeApp(App& app) {

        InitializeSceneController(app.sceneController);

    }
}
