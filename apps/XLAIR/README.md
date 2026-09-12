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
