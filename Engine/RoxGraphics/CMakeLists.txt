# ====== Define Source Subdirectories ======
set(SOURCE_SUBDIRS
    RoxFormats
    RoxLogger
    RoxMath
    RoxMemory
    RoxRender
    RoxResources
    RoxScene
)

# ====== Collect Source Files ======
set(SRC_FILES)

foreach(subdir IN LISTS SOURCE_SUBDIRS)
    # Add the subdirectory
    add_subdirectory(${subdir})
endforeach()

# ====== Define the Main Library ======
# Choose STATIC or SHARED based on your needs


# ====== Link Libraries ======
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    RoxFormats
    RoxLogger
    RoxMath
    RoxMemory
    RoxRender
    RoxResources
    RoxScene

    # External Libraries
)

# ====== Organize Sources in IDE ======
# (Optional) You can remove or adjust this if not used
source_group(TREE ${CMAKE_SOURCE_DIR} PREFIX "Graphics" FILES ${SRC_FILES})
