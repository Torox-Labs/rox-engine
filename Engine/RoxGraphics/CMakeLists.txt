# ====== Define Source Subdirectories ======
set(SOURCE_SUBDIRS
    RoxRender
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
    RoxRender
    RoxScene
    GLADInterface
    # External Libraries
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GLADInterface)


# ====== Organize Sources in IDE ======
# (Optional) You can remove or adjust this if not used
source_group(TREE ${CMAKE_SOURCE_DIR} PREFIX "Graphics" FILES ${SRC_FILES})
