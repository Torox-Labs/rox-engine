# ====== Define Sources ======
file(GLOB_RECURSE UI_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(ui STATIC ${UI_SRC})

# ====== Include Directories ======
target_include_directories(ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/ui
)

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${UI_SRC})
