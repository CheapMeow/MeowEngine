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

add_library(imguizmo INTERFACE)
target_include_directories(imguizmo
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ImGuizmo)

include(FetchContent)

option(QJS_BUILD_LIBC "" ON)

FetchContent_Declare(quickjs
                     GIT_REPOSITORY https://github.com/quickjs-ng/quickjs)

FetchContent_MakeAvailable(quickjs)
