# ====== Define Sources ======
file(GLOB_RECURSE RESOURCES_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(RoxResources STATIC ${RESOURCES_SRC})

# ====== Include Directories ======
target_include_directories(RoxResources
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(RoxResources PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'resources' under 'Graphics' in the solution
set_target_properties(RoxResources PROPERTIES FOLDER "Core")