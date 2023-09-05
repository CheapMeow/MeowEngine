# Download dependencies by using FetchContent_Declare
# Use FetchContent_MakeAvailable only in those code parts where the dependency is actually needed

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG "origin/v1.0"
    GIT_SHALLOW ON
    GIT_PROGRESS ON)

FetchContent_Declare(rocket
    GIT_REPOSITORY https://github.com/tripleslash/rocket.git
    GIT_TAG "origin/master"
    GIT_SHALLOW ON
    GIT_PROGRESS ON)

set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG "origin/master"
    GIT_SHALLOW ON
    GIT_PROGRESS ON)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG "origin/master"
    GIT_SHALLOW ON
    GIT_PROGRESS ON)