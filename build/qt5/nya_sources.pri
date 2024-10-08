
NYA_ENGINE_PATH = $$shell_path($$PWD/../..)
INCLUDEPATH += $$NYA_ENGINE_PATH
# to compile '.o' files (like scene/shader.cpp and render/shader.cpp) in different subdirs
CONFIG += object_parallel_to_source
macx: QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-parameter -Wno-reorder
QMAKE_OBJECTIVE_CFLAGS_WARN_ON = -Wall -Wno-unused-parameter -Wno-reorder
QMAKE_OBJECTIVE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-parameter -Wno-reorder


SOURCES += \
    $${NYA_ENGINE_PATH}/formats/dds.cpp \
    $${NYA_ENGINE_PATH}/formats/ktx.cpp \
    $${NYA_ENGINE_PATH}/formats/math_expr_parser.cpp \
    $${NYA_ENGINE_PATH}/formats/nms.cpp \
    $${NYA_ENGINE_PATH}/formats/meta.cpp \
    $${NYA_ENGINE_PATH}/formats/nan.cpp \
    $${NYA_ENGINE_PATH}/formats/string_convert.cpp \
    $${NYA_ENGINE_PATH}/formats/text_parser.cpp \
    $${NYA_ENGINE_PATH}/formats/tga.cpp \
    $${NYA_ENGINE_PATH}/log/log.cpp \
    $${NYA_ENGINE_PATH}/log/composite_log.cpp \
    $${NYA_ENGINE_PATH}/log/output_stream.cpp \
    $${NYA_ENGINE_PATH}/log/plain_file_log.cpp \
    $${NYA_ENGINE_PATH}/log/stdout_log.cpp \
    $${NYA_ENGINE_PATH}/log/warning.cpp \
    $${NYA_ENGINE_PATH}/log/windbg_log.cpp \
    $${NYA_ENGINE_PATH}/math/aabb.cpp \
    $${NYA_ENGINE_PATH}/math/bezier.cpp \
    $${NYA_ENGINE_PATH}/math/constants.cpp \
    $${NYA_ENGINE_PATH}/math/frustum.cpp \
    $${NYA_ENGINE_PATH}/math/matrix.cpp \
    $${NYA_ENGINE_PATH}/math/quadtree.cpp \
    $${NYA_ENGINE_PATH}/math/quaternion.cpp \
    $${NYA_ENGINE_PATH}/memory/memory.cpp \
    $${NYA_ENGINE_PATH}/memory/mutex.cpp \
    $${NYA_ENGINE_PATH}/memory/tmp_buffer.cpp \
    $${NYA_ENGINE_PATH}/render/animation.cpp \
    $${NYA_ENGINE_PATH}/render/bitmap.cpp \
    $${NYA_ENGINE_PATH}/render/debug_draw.cpp \
    $${NYA_ENGINE_PATH}/render/fbo.cpp \
    $${NYA_ENGINE_PATH}/render/render.cpp \
    $${NYA_ENGINE_PATH}/render/shader.cpp \
    $${NYA_ENGINE_PATH}/render/shader_code_parser.cpp \
    $${NYA_ENGINE_PATH}/render/skeleton.cpp \
    $${NYA_ENGINE_PATH}/render/statistics.cpp \
    $${NYA_ENGINE_PATH}/render/texture.cpp \
    $${NYA_ENGINE_PATH}/render/transform.cpp \
    $${NYA_ENGINE_PATH}/render/vbo.cpp \
    $${NYA_ENGINE_PATH}/resources/composite_resources_provider.cpp \
    $${NYA_ENGINE_PATH}/resources/file_resources_provider.cpp \
    $${NYA_ENGINE_PATH}/resources/memory_resources_provider.cpp \
    $${NYA_ENGINE_PATH}/resources/resources.cpp \
    $${NYA_ENGINE_PATH}/scene/animation.cpp \
    $${NYA_ENGINE_PATH}/scene/camera.cpp \
    $${NYA_ENGINE_PATH}/scene/location.cpp \
    $${NYA_ENGINE_PATH}/scene/material.cpp \
    $${NYA_ENGINE_PATH}/scene/mesh.cpp \
    $${NYA_ENGINE_PATH}/scene/postprocess.cpp \
    $${NYA_ENGINE_PATH}/scene/particles_group.cpp \
    $${NYA_ENGINE_PATH}/scene/particles.cpp \
    $${NYA_ENGINE_PATH}/scene/scene.cpp \
    $${NYA_ENGINE_PATH}/scene/shader.cpp \
    $${NYA_ENGINE_PATH}/scene/texture.cpp \
    $${NYA_ENGINE_PATH}/scene/transform.cpp \
    $${NYA_ENGINE_PATH}/system/shaders_cache_provider.cpp \
    $${NYA_ENGINE_PATH}/system/system.cpp \
    $${NYA_ENGINE_PATH}/ui/list.cpp \
    $${NYA_ENGINE_PATH}/ui/panel.cpp \
    $${NYA_ENGINE_PATH}/ui/slider.cpp \
    $${NYA_ENGINE_PATH}/ui/ui.cpp

# !macx: SOURCES += $${NYA_ENGINE_PATH}/system/app.cpp
# macx: OBJECTIVE_SOURCES += $${NYA_ENGINE_PATH}/system/app.mm

HEADERS += \
    $${NYA_ENGINE_PATH}/formats/dds.h \
    $${NYA_ENGINE_PATH}/formats/ktx.h \
    $${NYA_ENGINE_PATH}/formats/math_expr_parser.h \
    $${NYA_ENGINE_PATH}/formats/meta.h \
    $${NYA_ENGINE_PATH}/formats/nms.h \
    $${NYA_ENGINE_PATH}/formats/nan.h \
    $${NYA_ENGINE_PATH}/formats/string_convert.h \
    $${NYA_ENGINE_PATH}/formats/text_parser.h \
    $${NYA_ENGINE_PATH}/formats/tga.h \
    $${NYA_ENGINE_PATH}/gl/glext.h \
    $${NYA_ENGINE_PATH}/gl/wglext.h \
    $${NYA_ENGINE_PATH}/log/log.h \
    $${NYA_ENGINE_PATH}/log/composite_log.h \
    $${NYA_ENGINE_PATH}/log/output_stream.h \
    $${NYA_ENGINE_PATH}/log/plain_file_log.h \
    $${NYA_ENGINE_PATH}/log/stdout_log.h \
    $${NYA_ENGINE_PATH}/log/warning.h \
    $${NYA_ENGINE_PATH}/log/windbg_log.h \
    $${NYA_ENGINE_PATH}/math/aabb.h \
    $${NYA_ENGINE_PATH}/math/angle.h \
    $${NYA_ENGINE_PATH}/math/bezier.h \
    $${NYA_ENGINE_PATH}/math/constants.h \
    $${NYA_ENGINE_PATH}/math/frustum.h \
    $${NYA_ENGINE_PATH}/math/matrix.h \
    $${NYA_ENGINE_PATH}/math/quadtree.h \
    $${NYA_ENGINE_PATH}/math/quaternion.h \
    $${NYA_ENGINE_PATH}/math/scalar.h \
    $${NYA_ENGINE_PATH}/math/simd.h \
    $${NYA_ENGINE_PATH}/math/vector.h \
    $${NYA_ENGINE_PATH}/memory/align_alloc.h \
    $${NYA_ENGINE_PATH}/memory/indexed_map.h \
    $${NYA_ENGINE_PATH}/memory/invalid_object.h \
    $${NYA_ENGINE_PATH}/memory/memory.h \
    $${NYA_ENGINE_PATH}/memory/memory_reader.h \
    $${NYA_ENGINE_PATH}/memory/memory_writer.h \
    $${NYA_ENGINE_PATH}/memory/mutex.h \
    $${NYA_ENGINE_PATH}/memory/non_copyable.h \
    $${NYA_ENGINE_PATH}/memory/optional.h \
    $${NYA_ENGINE_PATH}/memory/pool.h \
    $${NYA_ENGINE_PATH}/memory/shared_ptr.h \
    $${NYA_ENGINE_PATH}/memory/tag_list.h \
    $${NYA_ENGINE_PATH}/memory/tile_map.h \
    $${NYA_ENGINE_PATH}/memory/tmp_buffer.h \
    $${NYA_ENGINE_PATH}/render/animation.h \
    $${NYA_ENGINE_PATH}/render/bitmap.h \
    $${NYA_ENGINE_PATH}/render/debug_draw.h \
    $${NYA_ENGINE_PATH}/render/fbo.h \
    $${NYA_ENGINE_PATH}/render/render.h \
    $${NYA_ENGINE_PATH}/render/render_objects.h \
    $${NYA_ENGINE_PATH}/render/shader.h \
    $${NYA_ENGINE_PATH}/render/shader_code_parser.h \
    $${NYA_ENGINE_PATH}/render/skeleton.h \
	$${NYA_ENGINE_PATH}/render/statistics.h \
    $${NYA_ENGINE_PATH}/render/texture.h \
    $${NYA_ENGINE_PATH}/render/transform.h \
    $${NYA_ENGINE_PATH}/render/vbo.h \
    $${NYA_ENGINE_PATH}/resources/composite_resources_provider.h \
    $${NYA_ENGINE_PATH}/resources/file_resources_provider.h \
    $${NYA_ENGINE_PATH}/resources/memory_resources_provider.h \
    $${NYA_ENGINE_PATH}/resources/resources.h \
    $${NYA_ENGINE_PATH}/resources/shared_resources.h \
    $${NYA_ENGINE_PATH}/scene/animation.h \
    $${NYA_ENGINE_PATH}/scene/camera.h \
    $${NYA_ENGINE_PATH}/scene/location.h \
    $${NYA_ENGINE_PATH}/scene/material.h \
    $${NYA_ENGINE_PATH}/scene/mesh.h \
    $${NYA_ENGINE_PATH}/scene/postprocess.h \
    $${NYA_ENGINE_PATH}/scene/particles_group.h \
    $${NYA_ENGINE_PATH}/scene/particles.h \
    $${NYA_ENGINE_PATH}/scene/proxy.h \
    $${NYA_ENGINE_PATH}/scene/scene.h \
    $${NYA_ENGINE_PATH}/scene/shader.h \
    $${NYA_ENGINE_PATH}/scene/shared_resources.h \
    $${NYA_ENGINE_PATH}/scene/tags.h \
    $${NYA_ENGINE_PATH}/scene/texture.h \
    $${NYA_ENGINE_PATH}/scene/transform.h \
    $${NYA_ENGINE_PATH}/system/app.h \
    $${NYA_ENGINE_PATH}/system/button_codes.h \
    $${NYA_ENGINE_PATH}/system/shaders_cache_provider.h \
    $${NYA_ENGINE_PATH}/system/system.h \
    $${NYA_ENGINE_PATH}/ui/button.h \
    $${NYA_ENGINE_PATH}/ui/label.h \
    $${NYA_ENGINE_PATH}/ui/list.h \
    $${NYA_ENGINE_PATH}/ui/panel.h \
    $${NYA_ENGINE_PATH}/ui/slider.h \
    $${NYA_ENGINE_PATH}/ui/ui.h

