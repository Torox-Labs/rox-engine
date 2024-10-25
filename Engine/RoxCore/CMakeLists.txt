# ====== Define Source Subdirectories ======
set(SOURCE_SUBDIRS
    RoxSystem
    RoxApp
    RoxInput
)

# ====== Collect Source Files ======
set(SRC_FILES)

foreach(subdir IN LISTS SOURCE_SUBDIRS)
    # Add the subdirectory
    add_subdirectory(${subdir})
endforeach()

# ====== Define the Main Library ======
# Choose STATIC or SHARED based on your needs
# add_library(${PROJECT_NAME} SHARED rox_engine.cpp)

# ====== Link Libraries ======
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    RoxSystem
    RoxApp
    RoxInput
    # External Libraries
)

# ====== Organize Sources in IDE ======
# (Optional) You can remove or adjust this if not used
source_group(TREE ${CMAKE_SOURCE_DIR} PREFIX "Core" FILES ${SRC_FILES})
