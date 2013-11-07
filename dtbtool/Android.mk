ifeq ($(BOARD_KERNEL_SEPARATED_DT),true)
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	dtbtool.c

LOCAL_CFLAGS += \
	-Wall

LOCAL_MODULE := dtbTool
LOCAL_MODULE_TAGS := optional

include $(BUILD_HOST_EXECUTABLE)
endif
