# ====== Define Sources ======
file(GLOB_RECURSE UI_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(ui STATIC ${UI_SRC})

# ====== Include Directories ======
target_include_directories(ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/ui
)

set_target_properties(ui PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'tools' under 'Graphics' in the solution
set_target_properties(tools PROPERTIES FOLDER "Graphics")

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TOOLS_SRC})
