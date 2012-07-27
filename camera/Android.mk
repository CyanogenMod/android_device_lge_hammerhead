ifeq (mako,$(TARGET_DEVICE))

ifneq ($(USE_CAMERA_STUB),true)
    # When zero we link against libmmcamera; when 1, we dlopen libmmcamera.
    DLOPEN_LIBMMCAMERA:=1
    ifneq ($(BUILD_TINY_ANDROID),true)
      V4L2_BASED_LIBCAM := true

      LOCAL_PATH:= $(call my-dir)

      include $(CLEAR_VARS)

      LOCAL_CFLAGS:= -DDLOPEN_LIBMMCAMERA=$(DLOPEN_LIBMMCAMERA)

      #define BUILD_UNIFIED_CODE
      BUILD_UNIFIED_CODE := false

      LOCAL_CFLAGS += -DUSE_ION

      LOCAL_CFLAGS += -DCAMERA_ION_HEAP_ID=ION_CP_MM_HEAP_ID # 8660=SMI, Rest=EBI
      LOCAL_CFLAGS += -DCAMERA_ZSL_ION_HEAP_ID=ION_CP_MM_HEAP_ID
      LOCAL_CFLAGS += -DCAMERA_GRALLOC_HEAP_ID=GRALLOC_USAGE_PRIVATE_MM_HEAP
      LOCAL_CFLAGS += -DCAMERA_GRALLOC_FALLBACK_HEAP_ID=GRALLOC_USAGE_PRIVATE_IOMMU_HEAP
      LOCAL_CFLAGS += -DCAMERA_ION_FALLBACK_HEAP_ID=ION_IOMMU_HEAP_ID
      LOCAL_CFLAGS += -DCAMERA_ZSL_ION_FALLBACK_HEAP_ID=ION_IOMMU_HEAP_ID
      LOCAL_CFLAGS += -DCAMERA_GRALLOC_CACHING_ID=0

      ifeq ($(V4L2_BASED_LIBCAM),true)
          LOCAL_HAL_FILES := QCameraHAL.cpp QCameraHWI_Parm.cpp\
            QCameraHWI.cpp QCameraHWI_Preview.cpp \
            QCameraHWI_Record.cpp QCameraHWI_Still.cpp \
            QCameraHWI_Mem.cpp QCameraHWI_Display.cpp \
            QCameraStream.cpp QualcommCamera2.cpp QCameraParameters.cpp
      else
        LOCAL_HAL_FILES := QualcommCamera.cpp QualcommCameraHardware.cpp
      endif

      LOCAL_CFLAGS+= -DHW_ENCODE

      # if debug service layer and up , use stub camera!
      LOCAL_C_INCLUDES += \
        frameworks/base/services/camera/libcameraservice #

      LOCAL_SRC_FILES := $(MM_CAM_FILES) $(LOCAL_HAL_FILES)

      LOCAL_CFLAGS+= -DNUM_PREVIEW_BUFFERS=4 -D_ANDROID_

      # To Choose neon/C routines for YV12 conversion
      LOCAL_CFLAGS+= -DUSE_NEON_CONVERSION
      # Uncomment below line to enable smooth zoom
      #LOCAL_CFLAGS+= -DCAMERA_SMOOTH_ZOOM

       LOCAL_C_INCLUDES+= \
        $(TARGET_OUT_HEADERS)/mm-camera \
        $(TARGET_OUT_HEADERS)/mm-camera/common \
        $(TARGET_OUT_HEADERS)/mm-still \
        $(TARGET_OUT_HEADERS)/mm-still/jpeg \

      ifeq ($(V4L2_BASED_LIBCAM),true)
        LOCAL_C_INCLUDES+= hardware/qcom/media/mm-core/inc
        LOCAL_C_INCLUDES+= $(TARGET_OUT_HEADERS)/mm-still/mm-omx
        LOCAL_C_INCLUDES+= $(LOCAL_PATH)/mm-camera-interface
      endif

      LOCAL_C_INCLUDES+= hardware/qcom/display/libgralloc
      LOCAL_C_INCLUDES+= hardware/qcom/display/libgenlock
      LOCAL_C_INCLUDES+= hardware/qcom/media/libstagefrighthw


      ifeq ($(V4L2_BASED_LIBCAM),true)
        LOCAL_SHARED_LIBRARIES:= libutils libui libcamera_client liblog libcutils libmmjpeg libmmstillomx libimage-jpeg-enc-omx-comp
        LOCAL_SHARED_LIBRARIES += libmmcamera_interface2
      else
         LOCAL_SHARED_LIBRARIES:= libutils libui libcamera_client liblog libcutils libmmjpeg
      endif

      LOCAL_SHARED_LIBRARIES+= libgenlock libbinder
      ifneq ($(DLOPEN_LIBMMCAMERA),1)
        LOCAL_SHARED_LIBRARIES+= liboemcamera
      else
        LOCAL_SHARED_LIBRARIES+= libdl
      endif

      LOCAL_CFLAGS += -include bionic/libc/kernel/common/linux/socket.h

      LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
      LOCAL_MODULE:= camera.$(TARGET_BOARD_PLATFORM)
      LOCAL_MODULE_TAGS := optional
      include $(BUILD_SHARED_LIBRARY)

    endif # BUILD_TINY_ANDROID
endif # USE_CAMERA_STUB

ifeq ($(V4L2_BASED_LIBCAM),true)
include $(LOCAL_PATH)/mm-camera-interface/Android.mk
endif

endif
