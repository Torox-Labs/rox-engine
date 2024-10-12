# ====== Define Sources ======
file(GLOB_RECURSE LOGGER_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(logger STATIC ${LOGGER_SRC})

# ====== Include Directories ======
target_include_directories(logger
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/logger
)

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${LOGGER_SRC})
