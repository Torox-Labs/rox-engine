# ====== Define Sources ======
if(APPLE)
file(GLOB_RECURSE INPUT_SRC "*.cpp" "*.mm" "*.h")
else()
file(GLOB_RECURSE INPUT_SRC "*.cpp" "*.h")
endif()

# ====== Add Library ======
add_library(RoxInput STATIC ${INPUT_SRC})

# ====== Include Directories ======
target_include_directories(RoxInput
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(RoxInput PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'system' under 'Graphics' in the solution
set_target_properties(RoxInput PROPERTIES FOLDER "Core")
