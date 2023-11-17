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

if(NOT TARGET spdlog)
    option(SPDLOG_BUILD_EXAMPLE "" OFF)
    option(SPDLOG_INSTALL "" OFF)
    add_subdirectory(spdlog)
endif()

# need have installed Vulkan SDK
find_package(Vulkan REQUIRED)

if(NOT TARGET volk)
    add_subdirectory(volk)
endif()

if(NOT TARGET glslang)
    add_subdirectory(glslang)
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

if(NOT TARGET EnTT::EnTT)
    add_subdirectory(entt)
endif()