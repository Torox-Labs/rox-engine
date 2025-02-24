# ====== Add Library ======
set(GLAD_SRC ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c)

add_library(GLAD OBJECT 
    ${GLAD_SRC}
)

target_include_directories(GLAD PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/glad/include
)

# ====== Set Folder Properties for Visual Studio ======
set_target_properties(GLAD PROPERTIES FOLDER "_Dependencies")