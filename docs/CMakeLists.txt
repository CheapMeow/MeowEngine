option(BUILD_DOCS_USE_VENV "Generate Python virtual environment" ON)

find_package(Python COMPONENTS Interpreter REQUIRED)
if (BUILD_DOCS_USE_VENV)
    set(DOCS_VENV "${CMAKE_CURRENT_BINARY_DIR}/venv")
    message(STATUS "Creating Python venv at ${DOCS_VENV}")
    execute_process(COMMAND ${Python_EXECUTABLE} -m venv ${DOCS_VENV})
    set(ENV{VIRTUAL_ENV} ${DOCS_VENV})
    set(Python_FIND_VIRTUALENV FIRST)
    unset(Python_EXECUTABLE)
    find_package(Python COMPONENTS Interpreter REQUIRED)

    execute_process(
        COMMAND ${Python_EXECUTABLE} -m pip install --no-cache wheel
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
    execute_process(
        COMMAND ${Python_EXECUTABLE} -m pip install --no-cache -r requirements.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND_ERROR_IS_FATAL ANY
    )
endif()

execute_process(
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../helper/req_check.py requirements.txt
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY
)

find_program(SPHINX_EXECUTABLE sphinx-build HINTS ${DOCS_VENV}/Scripts ${DOCS_VENV}/bin REQUIRED)
message(STATUS "Found Sphinx: ${SPHINX_EXECUTABLE}")

# Find all the public headers from runtime and editor
file(GLOB_RECURSE RUNTIME_PUBLIC_HEADERS CONFIGURE_DEPENDS ${SRC_ROOT_DIR}/runtime/*.h)
file(GLOB_RECURSE EDITOR_PUBLIC_HEADERS CONFIGURE_DEPENDS ${SRC_ROOT_DIR}/editor/*.h)

# Variables that are used to replace "@VAR@" pattern in Doxyfile.in
set(DOXYGEN_INPUT_DIR ${PROJECT_SOURCE_DIR}/src)
set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
set(DOXYGEN_EXCLUDE_DIR ${SRC_ROOT_DIR}/3rdparty)

set(DOXYGEN_INDEX_FILE ${DOXYGEN_OUTPUT_DIR}/xml/index.xml)
set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

# Replace variables inside @@ with the current values
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

# Doxygen won't create this for us
file(MAKE_DIRECTORY ${DOXYGEN_OUTPUT_DIR})

# Only regenerate Doxygen when the Doxyfile or public headers change
add_custom_command(OUTPUT ${DOXYGEN_INDEX_FILE}
                   DEPENDS ${RUNTIME_PUBLIC_HEADERS} ${EDITOR_PUBLIC_HEADERS}
                   COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
                   MAIN_DEPENDENCY ${DOXYFILE_OUT} ${DOXYFILE_IN}
                   COMMENT "Generating docs"
                   VERBATIM)

# Nice named target so we can run the job easily
add_custom_target(Doxygen ALL DEPENDS ${DOXYGEN_INDEX_FILE})

set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/sphinx)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

# Only regenerate Sphinx when:
# - Doxygen has rerun
# - Our doc files have been updated
# - The Sphinx config has been updated
add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
                   COMMAND 
                     ${SPHINX_EXECUTABLE} -b html
                     # Tell Breathe where to find the Doxygen output
                     -Dbreathe_projects.MeowEngine=${DOXYGEN_OUTPUT_DIR}/xml
                   ${SPHINX_SOURCE} ${SPHINX_BUILD}
                   WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                   DEPENDS
                   # Other docs files you want to track should go here (or in some variable)
                   ${CMAKE_CURRENT_SOURCE_DIR}/index.rst
                   ${DOXYGEN_INDEX_FILE}
                   MAIN_DEPENDENCY ${SPHINX_SOURCE}/conf.py
                   COMMENT "Generating documentation with Sphinx")

# Nice named target so we can run the job easily
add_custom_target(Sphinx ALL DEPENDS ${SPHINX_INDEX_FILE})

# Add an install target to install the docs
include(GNUInstallDirs)
install(DIRECTORY ${SPHINX_BUILD}
DESTINATION ${CMAKE_INSTALL_DOCDIR})