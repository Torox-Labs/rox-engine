# ====== Define Sources ======
file(GLOB_RECURSE RENDER_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(render STATIC ${RENDER_SRC})

# ====== Include Directories ======
target_include_directories(render
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/render
)

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${RENDER_SRC})
