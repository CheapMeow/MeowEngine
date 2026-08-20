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
# QuickJS-NG (debugger branch) — the single source of both the JS engine and
# the native Debug Adapter Protocol server (the latter is compiled into `qjs`).
# MeowEngine uses this repository's debugger branch directly. Point
# MEOW_QUICKJS_NG_DIR at your quickjs-ng checkout (must be on the `debugger`
# branch) and MEOW_QUICKJS_NG_LIB_DIR at the directory holding its static
# libraries. We link the prebuilt `qjs.lib` / `qjs-libc.lib` rather than
# add_subdirectory()'ing the whole project, so quickjs-ng's extra executables
# (qjs-debug, run-test262, ...) stay out of MeowEngine's default build.
# (In a clean checkout these would be a submodule plus its build output.)
# ---------------------------------------------------------------------------
set(MEOW_QUICKJS_NG_DIR "E:/repo/quickjs-ng" CACHE PATH "Source root of quickjs-ng (debugger branch)")
set(MEOW_QUICKJS_NG_LIB_DIR "${MEOW_QUICKJS_NG_DIR}/build-ninja"
    CACHE PATH "Directory containing qjs.lib / qjs-libc.lib")

# Link the prebuilt static libraries by their full path. We intentionally avoid
# an IMPORTED target here: the Visual Studio multi-config generator resolves
# IMPORTED_LOCATION oddly and ends up passing the target name (e.g. "qjs_ng.lib")
# to the linker instead of the actual file path. These are CACHE variables so
# they are visible from sibling subdirectories (e.g. meow_runtime).
set(MEOW_QUICKJS_NG_QJS_LIB "${MEOW_QUICKJS_NG_LIB_DIR}/qjs.lib"
    CACHE PATH "Prebuilt qjs.lib (quickjs-ng debugger branch)")
set(MEOW_QUICKJS_NG_QJS_LIBC_LIB "${MEOW_QUICKJS_NG_LIB_DIR}/qjs-libc.lib"
    CACHE PATH "Prebuilt qjs-libc.lib (quickjs-ng debugger branch)")
