# ====== Define Sources ======
file(GLOB_RECURSE RESOURCES_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(resources STATIC ${RESOURCES_SRC})

# ====== Include Directories ======
target_include_directories(resources
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/resources
)

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${RESOURCES_SRC})
