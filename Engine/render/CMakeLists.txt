# ====== Define Sources ======
file(GLOB_RECURSE RENDER_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(render STATIC ${RENDER_SRC})

# ====== Include Directories ======
target_include_directories(render
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/render
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'render' under 'Graphics' in the solution
set_target_properties(render PROPERTIES FOLDER "Graphics")

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${RENDER_SRC})
