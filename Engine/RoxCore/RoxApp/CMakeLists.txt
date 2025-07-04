# ====== Define Sources ======
if(APPLE)
file(GLOB_RECURSE APP_SRC "*.cpp" "*.mm" "*.h")
else()
file(GLOB_RECURSE APP_SRC "*.cpp" "*.h")
endif()

# ====== Add Library ======
add_library(RoxApp STATIC ${APP_SRC})

# ====== Include Directories ======
target_include_directories(RoxApp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# ====== Link Interface Libraries ======
target_link_libraries(RoxApp PRIVATE GLAD)

# ====== Set Output Directories ======
set_target_properties(RoxApp PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
)

# ====== Organize Sources in IDE ======
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SYSTEM_SRC})

# ====== Set Folder Properties for Visual Studio ======
# This nests 'system' under 'Graphics' in the solution
set_target_properties(RoxApp PROPERTIES FOLDER "Core")
