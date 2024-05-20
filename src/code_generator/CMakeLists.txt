set(CODE_GENERATOR_HEADER_FILES
    parser.h)
set(CODE_GENERATOR_SOURCE_FILES
    main.cpp
    parser.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${CODE_GENERATOR_HEADER_FILES}
                                                      ${CODE_GENERATOR_SOURCE_FILES})

add_executable(${CODE_GENERATOR_NAME} ${CODE_GENERATOR_HEADER_FILES} ${CODE_GENERATOR_SOURCE_FILES})

set_target_properties(${CODE_GENERATOR_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(${CODE_GENERATOR_NAME} PROPERTIES FOLDER "Engine")

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${CODE_GENERATOR_NAME}
                       PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")
target_compile_options(${CODE_GENERATOR_NAME}
                       PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")

target_link_libraries(${CODE_GENERATOR_NAME} PUBLIC $ENV{LLVM_DIR}/lib/libclang.lib)
target_include_directories(${CODE_GENERATOR_NAME}
                           PUBLIC $ENV{LLVM_DIR}/include)