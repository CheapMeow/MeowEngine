# add_subdirectory would add 3rdparty source code into cmake build tree

if(NOT TARGET glm)
  option(BUILD_STATIC_LIBS "" ON)
  option(BUILD_TESTING "" OFF)
  option(GLM_TEST_ENABLE "" OFF)
  add_subdirectory(glm)
endif()

if(NOT TARGET glfw)
  option(GLFW_BUILD_EXAMPLES "" OFF)
  option(GLFW_BUILD_TESTS "" OFF)
  option(GLFW_BUILD_DOCS "" OFF)
  option(GLFW_INSTALL "" OFF)
  add_subdirectory(glfw)
endif()

# need have installed Vulkan SDK
find_package(Vulkan REQUIRED)

if(NOT TARGET volk)
  add_subdirectory(volk)
endif()

if(NOT TARGET spirv-cross-glsl)
  add_subdirectory(SPIRV-Cross)
endif()

if(NOT TARGET stb)
  include(stb.cmake)
endif()

if(NOT TARGET assimp)
  # assimp may have warning
  option(ASSIMP_WARNINGS_AS_ERRORS "Don't treat all warnings as errors." OFF)
  add_subdirectory(assimp)
endif()

if(NOT TARGET imgui)
  include(imgui.cmake)
endif()

add_library(imguizmo STATIC ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/GraphEditor.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/GraphEditor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/ImCurveEdit.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/ImCurveEdit.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/ImGradient.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/ImGradient.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/ImGuizmo.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/ImGuizmo.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/ImSequencer.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/ImSequencer.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo/ImZoomSlider.h)
target_include_directories(imguizmo
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo)
target_link_libraries(imguizmo PUBLIC imgui)
target_include_directories(imguizmo PUBLIC ${3RD_PARTY_ROOT_DIR}/imgui)

# ---------------------------------------------------------------------------
# QuickJS-NG (debugger branch) — brought in as a git submodule at
# src/3rdparty/quickjs-ng. It is the single source of both the JS engine and
# the native Debug Adapter Protocol server (compiled into the `qjs` library).
# We add it EXCLUDE_FROM_ALL so only the `qjs` / `qjs-libc` libraries we
# actually consume get built — quickjs-ng's own executables (qjs-debug,
# run-test262, qjsc, api-test, ...) stay out of MeowEngine's default build.
# The `qjs` target already exposes quickjs-ng's headers via its INTERFACE
# includes, so no extra include_directories() is needed by consumers.
# ---------------------------------------------------------------------------
option(QJS_BUILD_EXAMPLES "" OFF)
option(QJS_BUILD_CLI_STATIC "" OFF)
option(QJS_BUILD_CLI_WITH_MIMALLOC "" OFF)
option(QJS_BUILD_CLI_WITH_STATIC_MIMALLOC "" OFF)
option(QJS_ENABLE_INSTALL "" OFF)
# Keep the C standard library separate (quickjs-libc.c) so it is exposed as its
# own `qjs-libc` target we can link against (for the js_std_* helpers). An
# existing cache value of ON would fold it into `qjs` and drop the target, so
# force it OFF here.
set(QJS_BUILD_LIBC OFF CACHE BOOL "Build standard library as a separate qjs-libc target" FORCE)
add_subdirectory(quickjs-ng EXCLUDE_FROM_ALL)
