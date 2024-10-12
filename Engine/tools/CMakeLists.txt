# ====== Define Sources ======
file(GLOB_RECURSE TOOLS_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(tools STATIC ${TOOLS_SRC})

# ====== Include Directories ======
target_include_directories(tools
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/tools
)

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TOOLS_SRC})
