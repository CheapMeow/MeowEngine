# add_subdirectory would add 3rdparty source code into cmake build tree

set(3RD_PARTY_FOLDER 3rdparty)

if(NOT TARGET glm)
    option(BUILD_STATIC_LIBS "" ON)
    option(BUILD_TESTING "" OFF)
    option(GLM_TEST_ENABLE "" OFF)
    add_subdirectory(glm)
    set_target_properties(glm_static PROPERTIES FOLDER ${3RD_PARTY_FOLDER}/glm)
endif()

if(NOT TARGET glfw)
    option(GLFW_BUILD_EXAMPLES "" OFF)
    option(GLFW_BUILD_TESTS "" OFF)
    option(GLFW_BUILD_DOCS "" OFF)
    option(GLFW_INSTALL "" OFF)
    add_subdirectory(glfw)
    set_target_properties(glfw PROPERTIES FOLDER ${3RD_PARTY_FOLDER}/glfw)
    set_target_properties(update_mappings PROPERTIES FOLDER ${3RD_PARTY_FOLDER}/glfw)
endif()

if(NOT TARGET spdlog)
    option(SPDLOG_BUILD_EXAMPLE "" OFF)
    option(SPDLOG_INSTALL "" OFF)
    add_subdirectory(spdlog)
    set_target_properties(spdlog PROPERTIES FOLDER ${3RD_PARTY_FOLDER}/spdlog)
endif()

if(NOT TARGET volk)
    add_subdirectory(volk)
    set_target_properties(volk PROPERTIES FOLDER ${3RD_PARTY_FOLDER}/volk)
endif()

if(NOT TARGET glslang)
    add_subdirectory(glslang)
    set_target_properties(glslang PROPERTIES FOLDER ${3RD_PARTY_FOLDER}/glslang)
endif()