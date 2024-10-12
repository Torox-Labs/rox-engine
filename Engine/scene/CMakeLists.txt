# ====== Define Sources ======
file(GLOB_RECURSE SCENE_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(scene STATIC ${SCENE_SRC})

# ====== Include Directories ======
target_include_directories(scene
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/scene
)

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SCENE_SRC})
