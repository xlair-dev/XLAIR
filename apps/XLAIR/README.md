# XLAIR App

## Architecture

```
src/
├── Main.cpp  # Composition root.
├── core/     # Game rules and domain state
├── app/      # Application workflows and long-lived application state
├── infra/    # Adapters for external systems
└── ui/       # Siv3D scenes, components, renderers, themes, and visual assets
```

`Main.cpp` is the only place allowed to know concrete `infra` and `ui` types at the same time.

`Common.hpp` only aggregates common external headers such as Siv3D.hpp; it must not include headers from XLAIR architecture layers.

### Dependency direction:

```text
Main  -> app, infra, ui
ui    -> app, core
infra -> app, core
app   -> core
core  -> no XLAIR layer
```

### Dependency injection

Prefer explicit constructor injection.
Do not use global service locators or Siv3D add-ons to make domain and application services globally accessible.
Introduce an interface only at a real side-effect boundary; passing an immutable value is preferred when no ongoing abstraction is needed.

## Config

XLAIR reads TOML config from `config.toml` beside the executable, or beside `XLAIR.app` on macOS.
Copy `config.example.toml` to `config.toml`. The local file is ignored by Git.
When the local file does not exist, the example file is used as the default during builds.

### iPad controller prototype

Set `[controller] mode = "ipad"`, run the [iPad controller app](https://github.com/xlair-dev/controller-ios-poc), connect the iPad over USB, and run `iproxy 24800 24800` on the Mac. The game reconnects automatically and clears input on disconnect. The app must stay in the foreground. The adapter receives input only; it does not send game lighting to the iPad. Input transitions are queued and consumed one per frame, so rapid changes can introduce additional latency. Measure latency on the target hardware before using this for timing evaluation.
