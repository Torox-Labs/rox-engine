NYA_PATH := $(call my-dir)/../../../../../../../
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := nya_native
LOCAL_CFLAGS := -Wall
LOCAL_C_INCLUDES := $(NYA_PATH)
LOCAL_SRC_FILES := $(NYA_PATH)/tests/test_cube/test_cube.cpp
LOCAL_LDLIBS := -landroid -lEGL -lGLESv2 -llog
LOCAL_STATIC_LIBRARIES := nya 

include $(BUILD_SHARED_LIBRARY)

include $(NYA_PATH)/build/android_ndk/android.mk
