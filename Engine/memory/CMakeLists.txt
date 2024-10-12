# ====== Define Sources ======
file(GLOB_RECURSE MEMORY_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(memory STATIC ${MEMORY_SRC})

# ====== Include Directories ======
target_include_directories(memory
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/memory
)

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${MEMORY_SRC})
