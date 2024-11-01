# ====== Define Source Subdirectories ======
set(SOURCE_SUBDIRS
    RoxSystem
    RoxApp
    RoxInput
    RoxLogger
    RoxMemory
    RoxMath # This will be deleted on the future and send to RoxMath
    RoxFormats
    RoxResources
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
    RoxSystem
    RoxApp
    RoxInput
    RoxLogger
    RoxMemory
    RoxMath # This will be deleted on the future and send to RoxMath
    RoxFormats
    RoxResources
    # External Libraries
)

# ====== Organize Sources in IDE ======
# (Optional) You can remove or adjust this if not used
source_group(TREE ${CMAKE_SOURCE_DIR} PREFIX "Core" FILES ${SRC_FILES})
