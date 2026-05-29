# Gymon Engine

Gymon is a lightweight 2D/3D game engine written in modern C++ (C++20) on an
OpenGL backend. It follows a layered, application-driven architecture: a single
`Application` drives a stack of `Layer`s, an event system propagates window and
input events, and a renderer abstraction sits on top of OpenGL.

The included **Sandbox** application demonstrates both renderers and lets you
switch between them at runtime from the menu bar.

## Features

- **Application / Layer architecture** — overlayable layers with attach,
  update, ImGui-render and event hooks, driven by a fixed `Application` loop.
- **Event system** — window, key, mouse and application events dispatched
  through a type-safe `EventDispatcher`.
- **Input polling** — static `Input` API plus GLFW-mapped key/mouse codes.
- **Renderer abstraction** — `RendererAPI` / `RenderCommand` / `Renderer` over
  OpenGL via Glad, with vertex/index buffers, buffer layouts, vertex arrays and
  a GLSL shader system (with `#type` section preprocessing and uniform helpers).
- **2D renderer** — a batched `Renderer2D` for colored/textured/rotated quads,
  with an orthographic camera controller and live draw-call statistics.
- **3D rendering** — indexed `Mesh` primitives (cube, plane), a free-look
  perspective camera controller (WASD + mouse look + scroll zoom) and a Phong
  lighting shader.
- **Textures** — `Texture2D` loaded via stb_image.
- **Dear ImGui** integration (docking branch) through the GLFW + OpenGL3
  backends for in-engine tooling and debug UI.

## Technologies Used

- **C++20** — core language.
- **OpenGL 4.5** — rendering backend.
- **GLFW** — windowing and input.
- **Glad** — OpenGL function loader.
- **GLM** — vector/matrix math.
- **Dear ImGui** — immediate-mode GUI (docking branch).
- **stb_image** — image loading.
- **spdlog** — logging.
- **premake5** — project generation.

> Platform support is currently **Windows** (the `Platform/Windows` and
> `Platform/OpenGL` layers). The renderer is abstracted behind interfaces, so
> additional platforms/backends can be added without touching engine code.

## Getting Started

### Prerequisites

- Windows
- Visual Studio 2022 or newer (with the *Desktop development with C++* workload)

### Build

1. **Clone the repository (with submodules):**

   ```bash
   git clone --recursive https://github.com/7irelo/gymon.git
   ```

   If you already cloned without `--recursive`:

   ```bash
   git submodule update --init --recursive
   ```

2. **Generate the Visual Studio solution:** run `GenerateProjects.bat`
   (it invokes the bundled `vendor/bin/premake/premake5.exe`).

3. **Build:** open `Gymon.sln` in Visual Studio, set **Sandbox** as the startup
   project, choose a configuration (`Debug`, `Release` or `Dist`) and build.

   You can also build from the command line with MSBuild:

   ```bash
   msbuild Gymon.sln /p:Configuration=Debug /p:Platform=x64
   ```

4. **Run Sandbox.** Use the **Scene** menu to switch between the 2D and 3D demos.
   In the 3D scene, move with **WASD** (plus **Space**/**Shift** for up/down),
   hold the **right mouse button** to look around, and scroll to zoom.

## Project Structure

```
Gymon/                Engine (static library)
  src/Gymon/          Core, events, layers, renderer abstraction, cameras
  src/Platform/       Windows window/input + OpenGL implementations
  vendor/             GLFW, Glad, ImGui, glm, stb_image, spdlog
Sandbox/              Example client application (2D and 3D demo layers)
premake5.lua          Build configuration
GenerateProjects.bat  Generates the Visual Studio solution
```

## Writing a Client Application

Subclass `Gymon::Application`, push your layers, and define `CreateApplication`:

```cpp
#include <Gymon.h>
#include <Gymon/EntryPoint.h>

class MyApp : public Gymon::Application
{
public:
    MyApp() { PushLayer(new MyLayer()); }
};

Gymon::Application* Gymon::CreateApplication()
{
    return new MyApp();
}
```

## License

This project is licensed under the [MIT License](LICENSE).
