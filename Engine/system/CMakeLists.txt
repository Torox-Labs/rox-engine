# ====== Define Sources ======
file(GLOB_RECURSE SYSTEM_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(system STATIC ${SYSTEM_SRC})

# ====== Include Directories ======
target_include_directories(system
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(system PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'system' under 'Graphics' in the solution
set_target_properties(system PROPERTIES FOLDER "Graphics")

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SYSTEM_SRC})
