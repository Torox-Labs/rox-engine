# ====== Define Sources ======
file(GLOB_RECURSE RENDER_SRC "*.cpp" "*.h")

# ====== Add Library ======
add_library(RoxRender STATIC ${RENDER_SRC})

# ====== Include Directories ======
target_include_directories(RoxRender
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ====== Link Interface Libraries ======
target_link_libraries(RoxRender PRIVATE GLAD)


# ====== Set File Language To Objective C++ ======
if(APPLE)
    set_source_files_properties(${RENDER_SRC} PROPERTIES
        LANGUAGE OBJCXX
    )
endif()

# ====== Set Output Directories ======
set_target_properties(RoxRender PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
)

# ====== Set Folder Properties for Visual Studio ======
# This nests 'render' under 'Graphics' in the solution
set_target_properties(RoxRender PROPERTIES FOLDER "Graphics")

