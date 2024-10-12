# ====== Define Sources ======
file(GLOB_RECURSE RESOURCES_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(resources STATIC ${RESOURCES_SRC})

# ====== Include Directories ======
target_include_directories(resources
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include/resources
)

set_target_properties(resources PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'resources' under 'Graphics' in the solution
set_target_properties(resources PROPERTIES FOLDER "Graphics")

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${RESOURCES_SRC})
