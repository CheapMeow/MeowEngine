set(CODE_GENERATOR_HEADER_FILES
    parser.h)
set(CODE_GENERATOR_SOURCE_FILES
    main.cpp
    parser.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${CODE_GENERATOR_HEADER_FILES}
                                                      ${CODE_GENERATOR_SOURCE_FILES})

add_executable(${CODE_GENERATOR_NAME} ${CODE_GENERATOR_HEADER_FILES} ${CODE_GENERATOR_SOURCE_FILES})

target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${SRC_ROOT_DIR}/runtime)
target_include_directories(${CODE_GENERATOR_NAME} PUBLIC ${3RD_PARTY_ROOT_DIR}/glm)

function(get_target_include_directories TARGET VAR_NAME)  
    set(INCLUDE_DIRS "")  
    get_target_property(TMP_DIRS ${TARGET} INCLUDE_DIRECTORIES)    
    foreach(DIR ${TMP_DIRS})  
        # If DIR is a generator expression, there will be no expansion here
        # Here we assume they are direct paths 
        list(APPEND INCLUDE_DIRS "-I${DIR}")  
    endforeach()   
    set(${VAR_NAME} "${INCLUDE_DIRS}" PARENT_SCOPE)  
endfunction()  

get_target_include_directories(${CODE_GENERATOR_NAME} INCLUDE_PATH_COLLECTION)  

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

add_custom_command(
    OUTPUT ${SRC_ROOT_DIR}/runtime/generated/register_all.cpp
    COMMAND ${CODE_GENERATOR_NAME} ${INCLUDE_PATH_COLLECTION} "-S${SRC_ROOT_DIR}/runtime" "-O${SRC_ROOT_DIR}/runtime/generated"
    DEPENDS ${CODE_GENERATOR_NAME} always_rebuild
    COMMENT "Generating register_all.cpp"
)
add_custom_target(${GENERATED_FILE_TARGET_NAME}
    DEPENDS ${SRC_ROOT_DIR}/runtime/generated/register_all.cpp
)

# Dummy output which is never actually produced. Anything that depends on
# this will always be rebuilt.
add_custom_command(
    OUTPUT always_rebuild
    COMMAND cmake -E echo
)