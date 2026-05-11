# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

Visual Studio 2022 + MSVC required. Environment variables `VK_SDK_PATH`/`VULKAN_SDK` and `LLVM_DIR` must point to installed Vulkan SDK and LLVM directories. `glslangValidator` must be on PATH for shader compilation.

```shell
# Debug build
cmake -S . -B build/ -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug
cmake --build build/ --parallel 8 --config Debug

# Release build
cmake -S . -B build/ -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build/ --parallel 8 --config Release

# Debug with AddressSanitizer
cmake -S . -B build/ -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build/ --parallel 8 --config Debug

# Compile shaders (from builtin/shaders/)
glslangValidator -V input.frag -o input.frag.spv
```

Batch scripts `build_debug.bat`, `build_release.bat`, `build_debug_memory.bat` wrap the above.

## Architecture

**Top-level structure**: 4 CMake subprojects under `src/`:
- `code_generator` — executable that generates static reflection code from C++ headers
- `meow_runtime` — static library, the core engine
- `meow_editor` — editor executable (startup project), links runtime + ImGuizmo
- `meow_game` — standalone game executable, links runtime only

**Engine lifecycle**: `MeowRuntime::Init()` → `Start()` → `Tick(dt)` loop → `ShutDown()`. All subsystems are `shared_ptr<System>` stored in `g_runtime_context` (a global `RuntimeGlobalContext` struct). System order in tick matters but is currently hardcoded.

**Static reflection build step**: `CodeGenerator` runs as a custom CMake command during build. It uses `libclang` to parse all runtime/editor headers, looking for `[[reflectable_class()]]`, `[[reflectable_field()]]`, `[[reflectable_method()]]` attributes (defined in `core/reflect/macros.h`). It generates `src/meow_runtime/generated/register_all.cpp`, which registers types with `TypeDescriptor` for runtime string-based type/field/method access. This file is compiled into `MeowRuntime` — changing any reflected header triggers regeneration and relinking.

**SPIR-V shader reflection**: `ShaderFactory` loads compiled `.spv` files and uses `SPIRV-Cross` to extract descriptor set layouts, vertex attribute bindings, and buffer metadata. This drives pipeline layout creation. All shader uniform buffers use dynamic uniform buffers (32KB ring buffer per material).

**Render pass hierarchy**: `RenderPassBase` owns `vk::raii::RenderPass` and framebuffers. Two main rendering paths inherit from it:
- `ForwardPassBase` — PBR+IBL forward rendering: opaque meshes → skybox → translucent meshes, MSAA with optional resolve
- `DeferredPassBase` — G-Buffer (color/normal/position attachments) → lighting pass rendering a fullscreen quad, supports 64 point lights
- `ShadowMapPass` and `ComputeParticlePass` also exist

Editor-level passes (`ForwardPassEditor`, `DeferredPassEditor`) inherit from these bases and add ImGui/flame-graph/settings widgets. Game-level passes are simpler.

**Material/Shader pipeline**: `ShaderFactory` (builder pattern) → `Shader` (SPIR-V modules, pipeline layout, descriptor set layout metas). `MaterialFactory` → `Material` (owns VkPipeline, per-frame descriptor sets, per-object dynamic offsets, uniform buffer ring buffers). `Material` exposes `BindPipeline`, `BindBufferToDescriptorSet`, `BindImageToDescriptorSet`, `PopulateDynamicUniformBuffer`, `BindDescriptorSetToPipeline`.

**ECS-lite**: `GameObject` stores `refl_shared_ptr<Component>` — type-erased components looked up by string type name (via reflection). Components have `Start()`/`Tick()` and a weak pointer back to their parent game object. `LevelSystem` manages levels, each level owning game objects.

**GPU particles**: `ParticleSystem` manages `GPUParticle2D` instances. Each has a compute material and a graphics material. `ComputeParticlePass` dispatches the compute shader, then renders with instancing.

**Profiling**: RAII `Timer` objects (via `SCOPE_TIMER(name)` / `FUNCTION_TIMER()` macros) push scope timing data to `TimerSingleton`. Data is cleared each frame and displayed in the editor's flame graph widget.

**Key third-party deps**: Vulkan SDK (via `vk::raii`), GLFW, GLM, SPIRV-Cross, Assimp, ImGui (+ ImGuizmo), stb, QuickJS, Volk (Vulkan loader).
