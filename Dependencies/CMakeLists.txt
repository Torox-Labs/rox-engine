# ====== Add Library ======
add_library(GLADInterface INTERFACE)

set(GLAD_SRC ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c)

add_library(GLAD STATIC
    ${GLAD_SRC}
)

set_source_files_properties(${GLAD_SRC} PROPERTIES
    LANGUAGE C
)

target_include_directories(GLAD PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
)

target_include_directories(GLADInterface INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/glad/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(GLADInterface INTERFACE GLAD)

# ====== Set Folder Properties for Visual Studio ======
set_target_properties(GLAD PROPERTIES FOLDER "Dependencies")