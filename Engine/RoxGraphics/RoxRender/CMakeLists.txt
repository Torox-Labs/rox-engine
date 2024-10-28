# ====== Define Sources ======
file(GLOB_RECURSE RENDER_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(RoxRender STATIC ${RENDER_SRC})

# ====== Include Directories ======
target_include_directories(RoxRender
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ======  ======
if(APPLE)
    set_source_files_properties(${RENDER_SRC} PROPERTIES
        LANGUAGE OBJCXX
    )
endif()

# ======  ======
set_target_properties(RoxRender PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'render' under 'Graphics' in the solution
set_target_properties(RoxRender PROPERTIES FOLDER "Graphics")

