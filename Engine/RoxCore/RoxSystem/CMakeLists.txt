# ====== Define Sources ======
if(APPLE)
file(GLOB_RECURSE SYSTEM_SRC "*.cpp" "*.mm" "*.h")
else()
file(GLOB_RECURSE SYSTEM_SRC "*.cpp" "*.h")
endif()

# ====== Add Library ======
add_library(RoxSystem STATIC ${SYSTEM_SRC})

# ====== Include Directories ======
target_include_directories(RoxSystem
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

set_target_properties(RoxSystem PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'system' under 'Graphics' in the solution
set_target_properties(RoxSystem PROPERTIES FOLDER "Core")
