# ====== Define Sources ======
file(GLOB_RECURSE LOGGER_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(RoxLogger STATIC ${LOGGER_SRC})

# ====== Include Directories ======
target_include_directories(RoxLogger
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(RoxLogger PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'logger' under 'Graphics' in the solution
set_target_properties(RoxLogger PROPERTIES FOLDER "Core")