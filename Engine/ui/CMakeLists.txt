# ====== Define Sources ======
file(GLOB_RECURSE FORMATS_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(formats STATIC ${FORMATS_SRC})

# ====== Include Directories ======
target_include_directories(formats
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/ui
)

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${FORMATS_SRC})
