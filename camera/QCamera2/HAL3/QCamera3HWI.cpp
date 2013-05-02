/* Copyright (c) 2012-2013, The Linux Foundataion. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#define LOG_TAG "QCamera3HWI"

#include <cutils/properties.h>
#include <hardware/camera3.h>
#include <camera/CameraMetadata.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include <gralloc_priv.h>
#include "QCamera3HWI.h"
#include "QCamera3Mem.h"
#include "QCamera3Channel.h"

using namespace android;

//using namespace android;
namespace qcamera {
#define DATA_PTR(MEM_OBJ,INDEX) MEM_OBJ->getPtr( INDEX )
cam_capability_t *gCamCapability[MM_CAMERA_MAX_NUM_SENSORS];
parm_buffer_t *prevSettings;
const camera_metadata_t *gStaticMetadata;


camera3_device_ops_t QCamera3HardwareInterface::mCameraOps = {
    initialize:                         QCamera3HardwareInterface::initialize,
    configure_streams:                  QCamera3HardwareInterface::configure_streams,
    register_stream_buffers:            QCamera3HardwareInterface::register_stream_buffers,
    construct_default_request_settings: QCamera3HardwareInterface::construct_default_request_settings,
    process_capture_request:            QCamera3HardwareInterface::process_capture_request,
    get_metadata_vendor_tag_ops:        QCamera3HardwareInterface::get_metadata_vendor_tag_ops,
    dump:                               QCamera3HardwareInterface::dump,
};


/*===========================================================================
 * FUNCTION   : QCamera3HardwareInterface
 *
 * DESCRIPTION: constructor of QCamera3HardwareInterface
 *
 * PARAMETERS :
 *   @cameraId  : camera ID
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3HardwareInterface::QCamera3HardwareInterface(int cameraId)
    : mCameraId(cameraId),
      mCameraHandle(NULL),
      mCameraOpened(false),
      mCallbackOps(NULL)
{
    mCameraDevice.common.tag = HARDWARE_DEVICE_TAG;
    mCameraDevice.common.version = CAMERA_DEVICE_API_VERSION_2_0;
    //mCameraDevice.common.close = close_camera_device;
    mCameraDevice.ops = &mCameraOps;
    mCameraDevice.priv = this;
    gCamCapability[cameraId]->version = CAM_HAL_V3;
}

/*===========================================================================
 * FUNCTION   : ~QCamera3HardwareInterface
 *
 * DESCRIPTION: destructor of QCamera2HardwareInterface
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3HardwareInterface::~QCamera3HardwareInterface()
{
    closeCamera();
}

/*===========================================================================
 * FUNCTION   : openCamera
 *
 * DESCRIPTION: open camera
 *
 * PARAMETERS :
 *   @hw_device  : double ptr for camera device struct
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::openCamera(struct hw_device_t **hw_device)
{
    //int rc = NO_ERROR;
    int rc = 0;
    if (mCameraOpened) {
        *hw_device = NULL;
        return PERMISSION_DENIED;
    }

    rc = openCamera();
    if (rc == 0)
        *hw_device = &mCameraDevice.common;
    else
        *hw_device = NULL;
    return rc;
}

/*===========================================================================
 * FUNCTION   : openCamera
 *
 * DESCRIPTION: open camera
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::openCamera()
{
    if (mCameraHandle) {
        ALOGE("Failure: Camera already opened");
        return ALREADY_EXISTS;
    }
    mCameraHandle = camera_open(mCameraId);
    if (!mCameraHandle) {
        ALOGE("camera_open failed.");
        return UNKNOWN_ERROR;
    }

    mCameraOpened = true;

    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : closeCamera
 *
 * DESCRIPTION: close camera
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::closeCamera()
{
    int rc = NO_ERROR;

    rc = mCameraHandle->ops->close_camera(mCameraHandle->camera_handle);
    mCameraHandle = NULL;
    mCameraOpened = false;

    return rc;
}

/*===========================================================================
 * FUNCTION   : sendCaptureResult
 *
 * DESCRIPTION: send completed capture result metadata buffer along with possibly
 *              completed output stream buffers to the framework
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
void QCamera3HardwareInterface::sendCaptureResult(const struct camera3_callback_ops *,
                                                 const camera3_capture_result_t *result)
{
    //TODO - Implement
}

/*===========================================================================
 * FUNCTION   : notify
 *
 * DESCRIPTION: Asynchronous notification callback to framework
 *
 * PARAMETERS :
 *
 * RETURN     :
 *
 *
 *==========================================================================*/

void QCamera3HardwareInterface::notify(const struct camera3_callback_ops *,
                                       const camera3_notify_msg_t *msg)
{
    //TODO - Implement
}


/*===========================================================================
 * FUNCTION   : initialize
 *
 * DESCRIPTION: Initialize frameworks callback functions
 *
 * PARAMETERS :
 *   @callback_ops : callback function to frameworks
 *
 * RETURN     :
 *
 *==========================================================================*/
int QCamera3HardwareInterface::initialize(
        const struct camera3_callback_ops *callback_ops)
{
    mCallbackOps = callback_ops;

    //TODO:Create metadata channel and initialize it
    mMetadataChannel = new QCamera3MetadataChannel(mCameraHandle->camera_handle,
                                        mCameraHandle->ops, channelCbRoutine);
    if (mMetadataChannel == NULL) {
        ALOGE("%s: failed to allocate metadata channel", __func__);
        return -ENOMEM;
    }
    return mMetadataChannel->initialize();
}

/*===========================================================================
 * FUNCTION   : configureStreams
 *
 * DESCRIPTION: Reset HAL camera device processing pipeline and set up new input
 *              and output streams.
 *
 * PARAMETERS :
 *   @stream_list : streams to be configured
 *
 * RETURN     :
 *
 *==========================================================================*/
int QCamera3HardwareInterface::configureStreams(
        camera3_stream_configuration_t *streamList)
{
    // Sanity check stream_list
    if (streamList == NULL) {
        ALOGE("%s: NULL stream configuration", __func__);
        return BAD_VALUE;
    }

    if (streamList->streams == NULL) {
        ALOGE("%s: NULL stream list", __func__);
        return BAD_VALUE;
    }

    if (streamList->num_streams < 1) {
        ALOGE("%s: Bad number of streams requested: %d", __func__,
                streamList->num_streams);
        return BAD_VALUE;
    }

    camera3_stream_t *inputStream = NULL;
    for (size_t i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t *newStream = streamList->streams[i];
        if (newStream->stream_type == CAMERA3_STREAM_INPUT) {
            if (inputStream != NULL) {
                ALOGE("%s: Multiple input streams requested!", __func__);
                return BAD_VALUE;
            }
            inputStream = newStream;
        }
    }
    mInputStream = inputStream;

    /* TODO: Clean up no longer used streams, and maintain others if this
     * is not the 1st time configureStreams is called */

    /* TODO: Reconstruct/reset metadata stream/channel */

    /* Allocate channel objects for the requested streams */
    for (size_t i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t *newStream = streamList->streams[i];
        if (newStream->priv == NULL) {
            //New stream, construct channel

            switch (newStream->stream_type) {
            case CAMERA3_STREAM_INPUT:
                newStream->usage = GRALLOC_USAGE_HW_CAMERA_READ;
                newStream->max_buffers = QCamera3PicChannel::kMaxBuffers;
                break;
            case CAMERA3_STREAM_BIDIRECTIONAL:
                newStream->usage = GRALLOC_USAGE_HW_CAMERA_READ |
                    GRALLOC_USAGE_HW_CAMERA_WRITE;
                newStream->max_buffers = QCamera3RegularChannel::kMaxBuffers;
                break;
            case CAMERA3_STREAM_OUTPUT:
                newStream->usage = GRALLOC_USAGE_HW_CAMERA_WRITE;
                newStream->max_buffers = QCamera3RegularChannel::kMaxBuffers;
                break;
            default:
                ALOGE("%s: Invalid stream_type %d", __func__, newStream->stream_type);
                break;
            }

            if (newStream->stream_type == CAMERA3_STREAM_OUTPUT ||
                newStream->stream_type == CAMERA3_STREAM_BIDIRECTIONAL) {
                QCamera3Channel *channel;
                switch (newStream->format) {
                case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
                    channel = new QCamera3RegularChannel(mCameraHandle->camera_handle,
                            mCameraHandle->ops, channelCbRoutine, newStream);
                    if (channel == NULL) {
                        ALOGE("%s: allocation of channel failed", __func__);
                        return -ENOMEM;
                    }

                    newStream->priv = channel;
                    break;
                case HAL_PIXEL_FORMAT_BLOB:
                    channel = new QCamera3PicChannel(mCameraHandle->camera_handle,
                            mCameraHandle->ops, channelCbRoutine, newStream);
                    if (channel == NULL) {
                        ALOGE("%s: allocation of channel failed", __func__);
                        return -ENOMEM;
                    }

                    newStream->priv = channel;
                    break;

                //TODO: Add support for app consumed format?
                default:
                    ALOGE("%s: not a supported format 0x%x", __func__, newStream->format);
                    break;
                }
            }
        } else {
            // Channel already exists for this stream
            // Do nothing for now
        }
    }
    return 0;
}

/*===========================================================================
 * FUNCTION   : registerStreamBuffers
 *
 * DESCRIPTION: Register buffers for a given stream with the HAL device.
 *
 * PARAMETERS :
 *   @stream_list : streams to be configured
 *
 * RETURN     :
 *
 *==========================================================================*/
int QCamera3HardwareInterface::registerStreamBuffers(
        const camera3_stream_buffer_set_t *buffer_set)
{
    int rc = 0;
    if (buffer_set == NULL) {
        ALOGE("%s: Invalid buffer_set parameter.", __func__);
        return -EINVAL;
    }
    if (buffer_set->stream == NULL) {
        ALOGE("%s: Invalid stream parameter.", __func__);
        return -EINVAL;
    }
    if (buffer_set->num_buffers < 1) {
        ALOGE("%s: Invalid num_buffers %d.", __func__, buffer_set->num_buffers);
        return -EINVAL;
    }
    if (buffer_set->buffers == NULL) {
        ALOGE("%s: Invalid buffers parameter.", __func__);
        return -EINVAL;
    }

    for (size_t i = 0; i < buffer_set->num_buffers; i++) {
        camera3_stream_t *stream = buffer_set->stream;
        QCamera3Channel *channel = (QCamera3Channel *)stream->priv;

        if (stream->stream_type != CAMERA3_STREAM_OUTPUT) {
            ALOGE("%s: not yet support non output type stream", __func__);
            return -EINVAL;
        }
        rc = channel->registerBuffers(buffer_set->num_buffers, buffer_set->buffers);
        if (rc < 0) {
            ALOGE("%s: registerBUffers for stream %p failed", __func__, stream);
            return -ENODEV;
        }
    }
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : processCaptureRequest
 *
 * DESCRIPTION:
 *
 *
 * PARAMETERS :
 *   @request : request from framework to process
 *
 * RETURN     :
 *
 *==========================================================================*/
int QCamera3HardwareInterface::processCaptureRequest
                                (camera3_capture_request_t *request)
{
    /*For each frame, we have to set the requested settings*/
    int rc = NO_ERROR;
    rc = setFrameParameters(request->settings);
    /*do the actual processing*/
    return rc;
}

#define DATA_PTR(MEM_OBJ,INDEX) MEM_OBJ->getPtr( INDEX )
/*===========================================================================
 * FUNCTION   : initCapabilities
 *
 * DESCRIPTION: initialize camera capabilities in static data struct
 *
 * PARAMETERS :
 *   @cameraId  : camera Id
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::initCapabilities(int cameraId)
{
    int rc = 0;
    mm_camera_vtbl_t *cameraHandle = NULL;
    QCamera3HeapMemory *capabilityHeap = NULL;

    cameraHandle = camera_open(cameraId);
    if (!cameraHandle) {
        ALOGE("%s: camera_open failed", __func__);
        rc = -1;
        goto open_failed;
    }

    /* Allocate memory for capability buffer */
    capabilityHeap = new QCamera3HeapMemory();
    rc = capabilityHeap->allocate(1, sizeof(cam_capability_t), false);
    if(rc != OK) {
        ALOGE("%s: No memory for cappability", __func__);
        goto allocate_failed;
    }

    /* Map memory for capability buffer */
    memset(DATA_PTR(capabilityHeap,0), 0, sizeof(cam_capability_t));
    rc = cameraHandle->ops->map_buf(cameraHandle->camera_handle,
                                CAM_MAPPING_BUF_TYPE_CAPABILITY,
                                capabilityHeap->getFd(0),
                                sizeof(cam_capability_t));
    if(rc < 0) {
        ALOGE("%s: failed to map capability buffer", __func__);
        goto map_failed;
    }

    /* Query Capability */
    rc = cameraHandle->ops->query_capability(cameraHandle->camera_handle);
    if(rc < 0) {
        ALOGE("%s: failed to query capability",__func__);
        goto query_failed;
    }
    gCamCapability[cameraId] = (cam_capability_t *)malloc(sizeof(cam_capability_t));
    if (!gCamCapability[cameraId]) {
        ALOGE("%s: out of memory", __func__);
        goto query_failed;
    }
    memcpy(gCamCapability[cameraId], DATA_PTR(capabilityHeap,0),
                                        sizeof(cam_capability_t));
    rc = 0;

query_failed:
    cameraHandle->ops->unmap_buf(cameraHandle->camera_handle,
                            CAM_MAPPING_BUF_TYPE_CAPABILITY);
map_failed:
    capabilityHeap->deallocate();
    delete capabilityHeap;
allocate_failed:
    cameraHandle->ops->close_camera(cameraHandle->camera_handle);
    cameraHandle = NULL;
open_failed:
    return rc;
}

/*===========================================================================
 * FUNCTION   : initStaticMetadata
 *
 * DESCRIPTION: initialize the static metadata
 *
 * PARAMETERS :
 *   @cameraId  : camera Id
 *
 * RETURN     : int32_t type of status
 *              0  -- success
 *              non-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::initStaticMetadata(int cameraId)
{
    int rc = 0;
    android::CameraMetadata staticInfo;

    staticInfo.update(ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
                    &gCamCapability[cameraId]->min_focus_distance, 1);

    staticInfo.update(ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE,
                    &gCamCapability[cameraId]->hyper_focal_distance, 1);

    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
                      gCamCapability[cameraId]->focal_lengths,
                      gCamCapability[cameraId]->focal_lengths_count);


    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_APERTURES,
                      gCamCapability[cameraId]->apertures,
                      gCamCapability[cameraId]->apertures_count);

    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,
                gCamCapability[cameraId]->filter_densities,
                gCamCapability[cameraId]->filter_densities_count);


    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
                      (int*)gCamCapability[cameraId]->optical_stab_modes,
                      gCamCapability[cameraId]->optical_stab_modes_count);

    staticInfo.update(ANDROID_LENS_POSITION,
                      gCamCapability[cameraId]->lens_position,
                      sizeof(gCamCapability[cameraId]->lens_position)/ sizeof(float));

    static const int32_t lens_shading_map_size[] = {gCamCapability[cameraId]->lens_shading_map_size.width,
                                                    gCamCapability[cameraId]->lens_shading_map_size.height};
    staticInfo.update(ANDROID_LENS_INFO_SHADING_MAP_SIZE,
                      lens_shading_map_size,
                      sizeof(lens_shading_map_size)/sizeof(int32_t));

    staticInfo.update(ANDROID_LENS_INFO_SHADING_MAP, gCamCapability[cameraId]->lens_shading_map,
            sizeof(gCamCapability[cameraId]->lens_shading_map_size)/ sizeof(cam_dimension_t));

    static const int32_t geo_correction_map_size[] = {gCamCapability[cameraId]->geo_correction_map_size.width,
                                                            gCamCapability[cameraId]->geo_correction_map_size.height};
    staticInfo.update(ANDROID_LENS_INFO_GEOMETRIC_CORRECTION_MAP_SIZE,
            geo_correction_map_size,
            sizeof(geo_correction_map_size)/sizeof(int32_t));

    staticInfo.update(ANDROID_LENS_INFO_GEOMETRIC_CORRECTION_MAP,
                       gCamCapability[cameraId]->geo_correction_map,
            sizeof(gCamCapability[cameraId]->geo_correction_map_size)/ sizeof(cam_dimension_t));

    staticInfo.update(ANDROID_SENSOR_INFO_PHYSICAL_SIZE,
            gCamCapability[cameraId]->sensor_physical_size, 2);

    staticInfo.update(ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE,
            gCamCapability[cameraId]->exposure_time_range, 2);

    staticInfo.update(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION,
            &gCamCapability[cameraId]->max_frame_duration, 1);


    staticInfo.update(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
                     (int*)&gCamCapability[cameraId]->color_arrangement, 1);

    static const int32_t pixel_array_size[] = {gCamCapability[cameraId]->pixel_array_size.width,
                                               gCamCapability[cameraId]->pixel_array_size.height};
    staticInfo.update(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE,
                      pixel_array_size, 2);

    static const int32_t active_array_size[] = {gCamCapability[cameraId]->active_array_size.width,
                                                gCamCapability[cameraId]->active_array_size.height};
    staticInfo.update(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
                      active_array_size, 2);

    staticInfo.update(ANDROID_SENSOR_INFO_WHITE_LEVEL,
            &gCamCapability[cameraId]->white_level, 1);

    staticInfo.update(ANDROID_SENSOR_BLACK_LEVEL_PATTERN,
            gCamCapability[cameraId]->black_level_pattern, 4);

    staticInfo.update(ANDROID_FLASH_INFO_CHARGE_DURATION,
                      &gCamCapability[cameraId]->flash_charge_duration, 1);

    staticInfo.update(ANDROID_TONEMAP_MAX_CURVE_POINTS,
                      &gCamCapability[cameraId]->max_tone_map_curve_points, 1);

    staticInfo.update(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT,
                      (int*)&gCamCapability[cameraId]->max_face_detection_count, 1);

    staticInfo.update(ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT,
                      &gCamCapability[cameraId]->histogram_size, 1);

    staticInfo.update(ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT,
            &gCamCapability[cameraId]->max_histogram_count, 1);

    static const int32_t sharpness_map_size[] = {gCamCapability[cameraId]->sharpness_map_size.width,
                                                gCamCapability[cameraId]->sharpness_map_size.height};

    staticInfo.update(ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE,
            sharpness_map_size, sizeof(sharpness_map_size)/sizeof(int32_t));

    staticInfo.update(ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE,
            &gCamCapability[cameraId]->max_sharpness_map_value, 1);


    staticInfo.update(ANDROID_SCALER_AVAILABLE_RAW_MIN_DURATIONS,
                      &gCamCapability[cameraId]->raw_min_duration,
                       1);

    static const int32_t exposureCompensationRange[] = {gCamCapability[cameraId]->exposure_compensation_min,
                                                        gCamCapability[cameraId]->exposure_compensation_max};
    staticInfo.update(ANDROID_CONTROL_AE_COMPENSATION_RANGE,
            exposureCompensationRange,
            sizeof(exposureCompensationRange)/sizeof(int32_t));

    uint8_t lensFacing = (gCamCapability[cameraId]->position == CAM_POSITION_BACK) ?
            ANDROID_LENS_FACING_BACK : ANDROID_LENS_FACING_FRONT;
    staticInfo.update(ANDROID_LENS_FACING, &lensFacing, 1);

    gStaticMetadata = staticInfo.release();
    return rc;
}

/*===========================================================================
 * FUNCTION   : getCapabilities
 *
 * DESCRIPTION: query camera capabilities
 *
 * PARAMETERS :
 *   @cameraId  : camera Id
 *   @info      : camera info struct to be filled in with camera capabilities
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::getCamInfo(int cameraId,
                                    struct camera_info *info)
{
    int rc = 0;

    if (NULL == gCamCapability[cameraId]) {
        rc = initCapabilities(cameraId);
        if (rc < 0) {
            //pthread_mutex_unlock(&g_camlock);
            return rc;
        }
    }

    if (NULL == gStaticMetadata) {
        rc = initStaticMetadata(cameraId);
        if (rc < 0) {
            return rc;
        }
    }

    switch(gCamCapability[cameraId]->position) {
    case CAM_POSITION_BACK:
        info->facing = CAMERA_FACING_BACK;
        break;

    case CAM_POSITION_FRONT:
        info->facing = CAMERA_FACING_FRONT;
        break;

    default:
        ALOGE("%s:Unknown position type for camera id:%d", __func__, cameraId);
        rc = -1;
        break;
    }


    info->orientation = gCamCapability[cameraId]->sensor_mount_angle;

    info->static_camera_characteristics = gStaticMetadata;

    return rc;
}

/*===========================================================================
 * FUNCTION   : translateMetadata
 *
 * DESCRIPTION: translate the metadata into camera_metadata_t
 *
 * PARAMETERS : type of the request
 *
 *
 * RETURN     : success: camera_metadata_t*
 *              failure: NULL
 *
 *==========================================================================*/
camera_metadata_t* QCamera3HardwareInterface::translateMetadata(int type)
{
    if (mDefaultMetadata[type] != NULL) {
        return mDefaultMetadata[type];
    }
    //first time we are handling this request
    //fill up the metadata structure using the wrapper class
    android::CameraMetadata settings;
    //translate from cam_capability_t to camera_metadata_tag_t
    static const uint8_t requestType = ANDROID_REQUEST_TYPE_CAPTURE;
    settings.update(ANDROID_REQUEST_TYPE, &requestType, 1);

    /*control*/

    uint8_t controlIntent = 0;
    switch (type) {
      case CAMERA3_TEMPLATE_PREVIEW:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
        break;
      case CAMERA3_TEMPLATE_STILL_CAPTURE:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE;
        break;
      case CAMERA3_TEMPLATE_VIDEO_RECORD:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD;
        break;
      case CAMERA3_TEMPLATE_VIDEO_SNAPSHOT:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT;
        break;
      case CAMERA3_TEMPLATE_ZERO_SHUTTER_LAG:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG;
        break;
      default:
        controlIntent = ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM;
        break;
    }
    settings.update(ANDROID_CONTROL_CAPTURE_INTENT, &controlIntent, 1);

    settings.update(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &gCamCapability[mCameraId]->exposure_compensation_default, 1);

    static const uint8_t aeLock = ANDROID_CONTROL_AE_LOCK_OFF;
    settings.update(ANDROID_CONTROL_AE_LOCK, &aeLock, 1);

    static const uint8_t awbLock = ANDROID_CONTROL_AWB_LOCK_OFF;
    settings.update(ANDROID_CONTROL_AWB_LOCK, &awbLock, 1);

    static const uint8_t awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
    settings.update(ANDROID_CONTROL_AWB_MODE, &awbMode, 1);

    static const uint8_t controlMode = ANDROID_CONTROL_MODE_AUTO;
    settings.update(ANDROID_CONTROL_MODE, &controlMode, 1);

    static const uint8_t effectMode = ANDROID_CONTROL_EFFECT_MODE_OFF;
    settings.update(ANDROID_CONTROL_EFFECT_MODE, &effectMode, 1);

    static const uint8_t sceneMode = ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY; //similar to AUTO?
    settings.update(ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);

    /*flash*/
    static const uint8_t flashMode = ANDROID_FLASH_MODE_OFF;
    settings.update(ANDROID_FLASH_MODE, &flashMode, 1);


    /* lens */
    static const float default_aperture = gCamCapability[mCameraId]->apertures[0];
    settings.update(ANDROID_LENS_APERTURE, &default_aperture, 1);

    static const float default_filter_density = gCamCapability[mCameraId]->filter_densities[0];
    settings.update(ANDROID_LENS_FILTER_DENSITY, &default_filter_density, 1);

    static const float default_focal_length = gCamCapability[mCameraId]->focal_lengths[0];
    settings.update(ANDROID_LENS_FOCAL_LENGTH, &default_focal_length, 1);

    mDefaultMetadata[type] = settings.release();
    return mDefaultMetadata[type];
}

/*===========================================================================
 * FUNCTION   : setFrameParameters
 *
 * DESCRIPTION: set parameters per frame as requested in the metadata from
 *              framework
 *
 * PARAMETERS :
 *   @settings  : frame settings information from framework
 *
 *
 * RETURN     : success: NO_ERROR
 *              failure:
 *==========================================================================*/
int QCamera3HardwareInterface::setFrameParameters(const camera_metadata_t *settings)
{
    /*translate from camera_metadata_t type to parm_type_t*/
    int rc = 0;
    android::CameraMetadata frame_settings;
    if (settings == NULL && prevSettings == NULL) {
        /*settings cannot be null for the first request*/
        return BAD_VALUE;
    } else if (settings == NULL) {
        /*do nothing? we have already configured the settings previously*/
    } else{
        //reset the prevSettings
        if (prevSettings != NULL) {
            free(prevSettings);
            prevSettings = NULL;
        }
        rc = translateMetadataToParameters(settings);
    }
    /*set the parameters to backend*/
    return rc;
}

/*===========================================================================
 * FUNCTION   : translateMetadataToParameters
 *
 * DESCRIPTION: read from the camera_metadata_t and change to parm_type_t
 *
 *
 * PARAMETERS :
 *   @settings  : frame settings information from framework
 *
 *
 * RETURN     : success: NO_ERROR
 *              failure:
 *==========================================================================*/
int QCamera3HardwareInterface::translateMetadataToParameters
                                  (const camera_metadata_t *settings)
{
    return 0;
}

/*===========================================================================
 * FUNCTION   : channelCbRoutine
 *
 * DESCRIPTION: Callback handler for all channels (streams, as well as metadata)
 *
 * PARAMETERS :
 *   @frame  : frame information from mm-camera-interface
 *   @buffer : actual gralloc buffer to be returned to frameworks. NULL if metadata.
 *   @userdata: userdata
 *
 * RETURN     : NONE
 *==========================================================================*/
void QCamera3HardwareInterface::channelCbRoutine(mm_camera_buf_def_t *frame,
                camera3_stream_buffer_t *buffer, void *userdata)
{
    QCamera3HardwareInterface *hw = (QCamera3HardwareInterface *)userdata;
    if (hw == NULL) {
        ALOGE("%s: Invalid hw %p", __func__, hw);
        return;
    }

    //TODO: Gives frame and buffer to buffer aggregator.

    return;
}

/*===========================================================================
 * FUNCTION   : initialize
 *
 * DESCRIPTION: Pass framework callback pointers to HAL
 *
 * PARAMETERS :
 *
 *
 * RETURN     : Success : 0
 *              Failure: -ENODEV
 *==========================================================================*/

int QCamera3HardwareInterface::initialize(const struct camera3_device *device,
                                  const camera3_callback_ops_t *callback_ops)
{
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return -ENODEV;
    }

    return hw->initialize(callback_ops);
}

/*===========================================================================
 * FUNCTION   : configure_streams
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     : Success: 0
 *              Failure: -EINVAL (if stream configuration is invalid)
 *                       -ENODEV (fatal error)
 *==========================================================================*/

int QCamera3HardwareInterface::configure_streams(
        const struct camera3_device *device,
        camera3_stream_configuration_t *stream_list)
{
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return -ENODEV;
    }
    return hw->configureStreams(stream_list);
}

/*===========================================================================
 * FUNCTION   : register_stream_buffers
 *
 * DESCRIPTION: Register stream buffers with the device
 *
 * PARAMETERS :
 *
 * RETURN     :
 *==========================================================================*/
int QCamera3HardwareInterface::register_stream_buffers(
        const struct camera3_device *device,
        const camera3_stream_buffer_set_t *buffer_set)
{
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return -ENODEV;
    }
    return hw->registerStreamBuffers(buffer_set);
}

/*===========================================================================
 * FUNCTION   : construct_default_request_settings
 *
 * DESCRIPTION: Configure a settings buffer to meet the required use case
 *
 * PARAMETERS :
 *
 *
 * RETURN     : Success: Return valid metadata
 *              Failure: Return NULL
 *==========================================================================*/
const camera_metadata_t* QCamera3HardwareInterface::construct_default_request_settings
(const struct camera3_device *device, int type)
{

    camera_metadata_t* fwk_metadata = NULL;
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return NULL;
    }

    fwk_metadata = hw->translateMetadata(type);

    return fwk_metadata;
}

/*===========================================================================
 * FUNCTION   : process_capture_request
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *==========================================================================*/
int QCamera3HardwareInterface::process_capture_request(const struct camera3_device *device,
                                                       camera3_capture_request_t *request)
{
    /*TODO - Implement*/
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    int rc = NO_ERROR;
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return -ENODEV;
    }
    rc = hw->processCaptureRequest(request);
    return rc;
}

/*===========================================================================
 * FUNCTION   : get_metadata_vendor_tag_ops
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *==========================================================================*/

void QCamera3HardwareInterface::get_metadata_vendor_tag_ops(const struct camera3_device *,
                                                           vendor_tag_query_ops_t* ops)
{
    /*TODO - Implement*/
}

/*===========================================================================
 * FUNCTION   : dump
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *==========================================================================*/

void QCamera3HardwareInterface::dump(const struct camera3_device *, int fd)
{
    /*TODO - Implement*/
}

}; //end namespace qcamera
