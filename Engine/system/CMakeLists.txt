# ====== Define Sources ======
file(GLOB_RECURSE SYSTEM_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(system STATIC ${SYSTEM_SRC})

# ====== Include Directories ======
target_include_directories(system
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/system
)

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SYSTEM_SRC})
