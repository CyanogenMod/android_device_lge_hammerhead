LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libqcamera2_util

LOCAL_SRC_FILES := \
        QCameraQueue.cpp \
        QCameraCmdThread.cpp \

LOCAL_CFLAGS := -Wall -Werror

LOCAL_C_INCLUDES := \
        $(LOCAL_PATH)/../stack/common \

include $(BUILD_STATIC_LIBRARY)
