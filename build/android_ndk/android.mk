LOCAL_PATH := $(NYA_PATH)

include $(CLEAR_VARS)

LOCAL_MODULE := nya

LOCAL_C_INCLUDES := $(NYA_PATH)

LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES)

LOCAL_SRC_FILES := \
    $(wildcard $(LOCAL_PATH)/formats/*.cpp) \
    $(wildcard $(LOCAL_PATH)/log/*.cpp) \
    $(wildcard $(LOCAL_PATH)/math/*.cpp) \
    $(wildcard $(LOCAL_PATH)/memory/*.cpp) \
    $(wildcard $(LOCAL_PATH)/render/*.cpp) \
    $(wildcard $(LOCAL_PATH)/resources/*.cpp) \
    $(wildcard $(LOCAL_PATH)/scene/*.cpp) \
    $(wildcard $(LOCAL_PATH)/system/*.cpp) \
    $(wildcard $(LOCAL_PATH)/ui/*.cpp)

LOCAL_EXPORT_LDLIBS := -lEGL -lGLESv2

LOCAL_CPPFLAGS := -mfloat-abi=softfp -mfpu=neon

include $(BUILD_STATIC_LIBRARY)
