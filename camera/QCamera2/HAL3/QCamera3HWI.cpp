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
#include <ui/Fence.h>
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

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::EFFECT_MODES_MAP[] = {
    { ANDROID_CONTROL_EFFECT_MODE_OFF,       CAM_EFFECT_MODE_OFF },
    { ANDROID_CONTROL_EFFECT_MODE_MONO,       CAM_EFFECT_MODE_MONO },
    { ANDROID_CONTROL_EFFECT_MODE_NEGATIVE,   CAM_EFFECT_MODE_NEGATIVE },
    { ANDROID_CONTROL_EFFECT_MODE_SOLARIZE,   CAM_EFFECT_MODE_SOLARIZE },
    { ANDROID_CONTROL_EFFECT_MODE_SEPIA,      CAM_EFFECT_MODE_SEPIA },
    { ANDROID_CONTROL_EFFECT_MODE_POSTERIZE,  CAM_EFFECT_MODE_POSTERIZE },
    { ANDROID_CONTROL_EFFECT_MODE_WHITEBOARD, CAM_EFFECT_MODE_WHITEBOARD },
    { ANDROID_CONTROL_EFFECT_MODE_BLACKBOARD, CAM_EFFECT_MODE_BLACKBOARD },
    { ANDROID_CONTROL_EFFECT_MODE_AQUA,       CAM_EFFECT_MODE_AQUA }
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::WHITE_BALANCE_MODES_MAP[] = {
    { ANDROID_CONTROL_AWB_MODE_AUTO,            CAM_WB_MODE_AUTO },
    { ANDROID_CONTROL_AWB_MODE_INCANDESCENT,    CAM_WB_MODE_INCANDESCENT },
    { ANDROID_CONTROL_AWB_MODE_FLUORESCENT,     CAM_WB_MODE_FLUORESCENT },
    { ANDROID_CONTROL_AWB_MODE_WARM_FLUORESCENT,CAM_WB_MODE_WARM_FLUORESCENT},
    { ANDROID_CONTROL_AWB_MODE_DAYLIGHT,        CAM_WB_MODE_DAYLIGHT },
    { ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT, CAM_WB_MODE_CLOUDY_DAYLIGHT },
    { ANDROID_CONTROL_AWB_MODE_TWILIGHT,        CAM_WB_MODE_TWILIGHT },
    { ANDROID_CONTROL_AWB_MODE_SHADE,           CAM_WB_MODE_SHADE }
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::SCENE_MODES_MAP[] = {
    { ANDROID_CONTROL_SCENE_MODE_UNSUPPORTED,    CAM_SCENE_MODE_OFF },
    { ANDROID_CONTROL_SCENE_MODE_ACTION,         CAM_SCENE_MODE_ACTION },
    { ANDROID_CONTROL_SCENE_MODE_PORTRAIT,       CAM_SCENE_MODE_PORTRAIT },
    { ANDROID_CONTROL_SCENE_MODE_LANDSCAPE,      CAM_SCENE_MODE_LANDSCAPE },
    { ANDROID_CONTROL_SCENE_MODE_NIGHT,          CAM_SCENE_MODE_NIGHT },
    { ANDROID_CONTROL_SCENE_MODE_NIGHT_PORTRAIT, CAM_SCENE_MODE_NIGHT_PORTRAIT },
    { ANDROID_CONTROL_SCENE_MODE_THEATRE,        CAM_SCENE_MODE_THEATRE },
    { ANDROID_CONTROL_SCENE_MODE_BEACH,          CAM_SCENE_MODE_BEACH },
    { ANDROID_CONTROL_SCENE_MODE_SNOW,           CAM_SCENE_MODE_SNOW },
    { ANDROID_CONTROL_SCENE_MODE_SUNSET,         CAM_SCENE_MODE_SUNSET },
    { ANDROID_CONTROL_SCENE_MODE_STEADYPHOTO,    CAM_SCENE_MODE_ANTISHAKE },
    { ANDROID_CONTROL_SCENE_MODE_FIREWORKS ,     CAM_SCENE_MODE_FIREWORKS },
    { ANDROID_CONTROL_SCENE_MODE_SPORTS ,        CAM_SCENE_MODE_SPORTS },
    { ANDROID_CONTROL_SCENE_MODE_PARTY,          CAM_SCENE_MODE_PARTY },
    { ANDROID_CONTROL_SCENE_MODE_CANDLELIGHT,    CAM_SCENE_MODE_CANDLELIGHT },
    { ANDROID_CONTROL_SCENE_MODE_BARCODE,        CAM_SCENE_MODE_OFF}
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::FOCUS_MODES_MAP[] = {
    { ANDROID_CONTROL_AF_MODE_AUTO,               CAM_FOCUS_MODE_AUTO },
    { ANDROID_CONTROL_AF_MODE_MACRO,              CAM_FOCUS_MODE_MACRO },
    { ANDROID_CONTROL_AF_MODE_EDOF,               CAM_FOCUS_MODE_EDOF },
    { ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE, CAM_FOCUS_MODE_CONTINOUS_PICTURE },
    { ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO,   CAM_FOCUS_MODE_CONTINOUS_VIDEO }
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::ANTIBANDING_MODES_MAP[] = {
    { ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF,  CAM_ANTIBANDING_MODE_OFF },
    { ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ, CAM_ANTIBANDING_MODE_50HZ },
    { ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ, CAM_ANTIBANDING_MODE_60HZ },
    { ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO, CAM_ANTIBANDING_MODE_AUTO }
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::AUTO_EXPOSURE_MAP[] = {
    { ANDROID_CONTROL_AE_MODE_OFF,    CAM_AEC_MODE_FRAME_AVERAGE },
    { ANDROID_CONTROL_AE_MODE_ON,     CAM_AEC_MODE_FRAME_AVERAGE },
};

const QCamera3HardwareInterface::QCameraMap QCamera3HardwareInterface::FLASH_MODES_MAP[] = {
    { ANDROID_FLASH_MODE_OFF,    CAM_FLASH_MODE_OFF  },
    { ANDROID_FLASH_MODE_SINGLE, CAM_FLASH_MODE_ON   },
    { ANDROID_FLASH_MODE_TORCH,  CAM_FLASH_MODE_TORCH}
};


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
    mCameraDevice.common.version = CAMERA_DEVICE_API_VERSION_3_0;
    mCameraDevice.common.close = close_camera_device;
    mCameraDevice.ops = &mCameraOps;
    mCameraDevice.priv = this;
    gCamCapability[cameraId]->version = CAM_HAL_V3;

    pthread_mutex_init(&mRequestLock, NULL);
    pthread_cond_init(&mRequestCond, NULL);
    mPendingRequest = 0;

    pthread_mutex_init(&mMutex, NULL);
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

    pthread_mutex_destroy(&mRequestLock);
    pthread_cond_destroy(&mRequestCond);

    pthread_mutex_destroy(&mMutex);
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
    int rc;

    pthread_mutex_lock(&mMutex);

    //Create metadata channel and initialize it
    mMetadataChannel = new QCamera3MetadataChannel(mCameraHandle->camera_handle,
                    mCameraHandle->ops, captureResultCb,
                    &gCamCapability[mCameraId]->padding_info, this);
    if (mMetadataChannel == NULL) {
        ALOGE("%s: failed to allocate metadata channel", __func__);
        rc = -ENOMEM;
        goto err1;
    }
    rc = mMetadataChannel->initialize();
    if (rc < 0) {
        ALOGE("%s: metadata channel initialization failed", __func__);
        goto err2;
    }

    /* Initialize parameter heap and structure */
    mParamHeap = new QCamera3HeapMemory();
    if (mParamHeap == NULL) {
        ALOGE("%s: creation of mParamHeap failed", __func__);
        goto err2;
    }
    rc = mParamHeap->allocate(1, sizeof(parm_buffer_t), false);
    if (rc < 0) {
        ALOGE("%s: allocation of mParamHeap failed", __func__);
        goto err3;
    }
    rc = mCameraHandle->ops->map_buf(mCameraHandle->camera_handle,
                CAM_MAPPING_BUF_TYPE_PARM_BUF,
                mParamHeap->getFd(0), sizeof(parm_buffer_t));
    if (rc < 0) {
        ALOGE("%s: map_buf failed for mParamHeap", __func__);
        goto err4;
    }
    mParameters = (parm_buffer_t *)DATA_PTR(mParamHeap, 0);

    mCallbackOps = callback_ops;

    pthread_mutex_unlock(&mMutex);
    return 0;

err4:
    mParamHeap->deallocate();
err3:
    delete mParamHeap;
    mParamHeap = NULL;
err2:
    delete mMetadataChannel;
    mMetadataChannel = NULL;
err1:
    pthread_mutex_unlock(&mMutex);
    return rc;
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
    pthread_mutex_lock(&mMutex);

    // Sanity check stream_list
    if (streamList == NULL) {
        ALOGE("%s: NULL stream configuration", __func__);
        pthread_mutex_unlock(&mMutex);
        return BAD_VALUE;
    }

    if (streamList->streams == NULL) {
        ALOGE("%s: NULL stream list", __func__);
        pthread_mutex_unlock(&mMutex);
        return BAD_VALUE;
    }

    if (streamList->num_streams < 1) {
        ALOGE("%s: Bad number of streams requested: %d", __func__,
                streamList->num_streams);
        pthread_mutex_unlock(&mMutex);
        return BAD_VALUE;
    }

    camera3_stream_t *inputStream = NULL;
    for (size_t i = 0; i < streamList->num_streams; i++) {
        camera3_stream_t *newStream = streamList->streams[i];
        if (newStream->stream_type == CAMERA3_STREAM_INPUT) {
            if (inputStream != NULL) {
                ALOGE("%s: Multiple input streams requested!", __func__);
                pthread_mutex_unlock(&mMutex);
                return BAD_VALUE;
            }
            inputStream = newStream;
        }
    }
    mInputStream = inputStream;

    /* TODO: Clean up no longer used streams, and maintain others if this
     * is not the 1st time configureStreams is called */

    mMetadataChannel->stop();

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
                            mCameraHandle->ops, captureResultCb,
                            &gCamCapability[mCameraId]->padding_info, this, newStream);
                    if (channel == NULL) {
                        ALOGE("%s: allocation of channel failed", __func__);
                        pthread_mutex_unlock(&mMutex);
                        return -ENOMEM;
                    }

                    newStream->priv = channel;
                    break;
                case HAL_PIXEL_FORMAT_BLOB:
                    channel = new QCamera3PicChannel(mCameraHandle->camera_handle,
                            mCameraHandle->ops, captureResultCb,
                            &gCamCapability[mCameraId]->padding_info, this, newStream);
                    if (channel == NULL) {
                        ALOGE("%s: allocation of channel failed", __func__);
                        pthread_mutex_unlock(&mMutex);
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

    // Cannot reuse settings across configure call
    memset(mParameters, 0, sizeof(parm_buffer_t));
    pthread_mutex_unlock(&mMutex);
    return 0;
}

/*===========================================================================
 * FUNCTION   : validateCaptureRequest
 *
 * DESCRIPTION: validate a capture request from camera service
 *
 * PARAMETERS :
 *   @request : request from framework to process
 *
 * RETURN     :
 *
 *==========================================================================*/
int QCamera3HardwareInterface::validateCaptureRequest(
                    camera3_capture_request_t *request)
{
    int rc = NO_ERROR;
    ssize_t idx = 0;
    const camera3_stream_buffer_t *b;
    CameraMetadata meta;

    /* Sanity check the request */
    if (request == NULL) {
        ALOGE("%s: NULL capture request", __func__);
        return BAD_VALUE;
    }

    uint32_t frameNumber = request->frame_number;
    if (request->input_buffer != NULL &&
            request->input_buffer->stream != mInputStream) {
        ALOGE("%s: Request %d: Input buffer not from input stream!",
                __FUNCTION__, frameNumber);
        return BAD_VALUE;
    }
    if (request->num_output_buffers < 1 || request->output_buffers == NULL) {
        ALOGE("%s: Request %d: No output buffers provided!",
                __FUNCTION__, frameNumber);
        return BAD_VALUE;
    }
    if (request->input_buffer != NULL) {
        //TODO
        ALOGE("%s: Not supporting input buffer yet", __func__);
        return BAD_VALUE;
    }

    // Validate all buffers
    b = request->output_buffers;
    do {
        QCamera3Channel *channel =
                static_cast<QCamera3Channel*>(b->stream->priv);
        if (channel == NULL) {
            ALOGE("%s: Request %d: Buffer %d: Unconfigured stream!",
                    __func__, frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->status != CAMERA3_BUFFER_STATUS_OK) {
            ALOGE("%s: Request %d: Buffer %d: Status not OK!",
                    __func__, frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->release_fence != -1) {
            ALOGE("%s: Request %d: Buffer %d: Has a release fence!",
                    __func__, frameNumber, idx);
            return BAD_VALUE;
        }
        if (b->buffer == NULL) {
            ALOGE("%s: Request %d: Buffer %d: NULL buffer handle!",
                    __func__, frameNumber, idx);
            return BAD_VALUE;
        }
        idx++;
        b = request->output_buffers + idx;
    } while (idx < (ssize_t)request->num_output_buffers);

    return NO_ERROR;
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

    pthread_mutex_lock(&mMutex);

    if (buffer_set == NULL) {
        ALOGE("%s: Invalid buffer_set parameter.", __func__);
        pthread_mutex_unlock(&mMutex);
        return -EINVAL;
    }
    if (buffer_set->stream == NULL) {
        ALOGE("%s: Invalid stream parameter.", __func__);
        pthread_mutex_unlock(&mMutex);
        return -EINVAL;
    }
    if (buffer_set->num_buffers < 1) {
        ALOGE("%s: Invalid num_buffers %d.", __func__, buffer_set->num_buffers);
        pthread_mutex_unlock(&mMutex);
        return -EINVAL;
    }
    if (buffer_set->buffers == NULL) {
        ALOGE("%s: Invalid buffers parameter.", __func__);
        pthread_mutex_unlock(&mMutex);
        return -EINVAL;
    }

    camera3_stream_t *stream = buffer_set->stream;
    QCamera3Channel *channel = (QCamera3Channel *)stream->priv;

    if (stream->stream_type != CAMERA3_STREAM_OUTPUT) {
        ALOGE("%s: not yet support non output type stream", __func__);
        pthread_mutex_unlock(&mMutex);
        return -EINVAL;
    }
    rc = channel->registerBuffers(buffer_set->num_buffers, buffer_set->buffers);
    if (rc < 0) {
        ALOGE("%s: registerBUffers for stream %p failed", __func__, stream);
        pthread_mutex_unlock(&mMutex);
        return -ENODEV;
    }

    pthread_mutex_unlock(&mMutex);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : processCaptureRequest
 *
 * DESCRIPTION: process a capture request from camera service
 *
 * PARAMETERS :
 *   @request : request from framework to process
 *
 * RETURN     :
 *
 *==========================================================================*/
int QCamera3HardwareInterface::processCaptureRequest(
                    camera3_capture_request_t *request)
{
    int rc = NO_ERROR;
    ssize_t idx = 0;
    const camera3_stream_buffer_t *b;
    CameraMetadata meta;

    pthread_mutex_lock(&mMutex);

    rc = validateCaptureRequest(request);
    if (rc != NO_ERROR) {
        ALOGE("%s: incoming request is not valid", __func__);
        pthread_mutex_unlock(&mMutex);
        return rc;
    }

    uint32_t frameNumber = request->frame_number;

    rc = setFrameParameters(request->frame_number, request->settings);

    if (rc < 0) {
        ALOGE("%s: fail to set frame parameters", __func__);
        pthread_mutex_unlock(&mMutex);
        return rc;
    }

    // Acquire all request buffers first
    for (size_t i = 0; i < request->num_output_buffers; i++) {
        const camera3_stream_buffer_t& output = request->output_buffers[i];
        sp<Fence> acquireFence = new Fence(output.acquire_fence);
        rc = acquireFence->wait(Fence::TIMEOUT_NEVER);
        if (rc != OK) {
            ALOGE("%s: fence wait failed %d", __func__, rc);
            pthread_mutex_unlock(&mMutex);
            return rc;
        }
    }

    // Notify metadata channel we receive a request
    mMetadataChannel->request(NULL, frameNumber);

    // Call request on other streams
    for (size_t i = 0; i < request->num_output_buffers; i++) {
        const camera3_stream_buffer_t& output = request->output_buffers[i];
        QCamera3Channel *channel = (QCamera3Channel *)output.stream->priv;
        if (channel == NULL) {
            ALOGE("%s: invalid channel pointer for stream", __func__);
            continue;
        }

        rc = channel->request(output.buffer, frameNumber);
        if (rc < 0)
            ALOGE("%s: request failed", __func__);
    }

    //Block on conditional variable
    pthread_mutex_lock(&mRequestLock);
    mPendingRequest = 1;
    while (mPendingRequest == 1) {
        pthread_cond_wait(&mRequestCond, &mRequestLock);
    }
    pthread_mutex_unlock(&mRequestLock);

    pthread_mutex_unlock(&mMutex);
    return rc;
}

/*===========================================================================
 * FUNCTION   : getMetadataVendorTagOps
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *==========================================================================*/
void QCamera3HardwareInterface::getMetadataVendorTagOps(vendor_tag_query_ops_t* ops)
{
    /* Enable locks when we eventually add Vendor Tags */
    /*
    pthread_mutex_lock(&mMutex);

    pthread_mutex_unlock(&mMutex);
    */
    return;
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
void QCamera3HardwareInterface::dump(int fd)
{
    /*Enable lock when we implement this function*/
    /*
    pthread_mutex_lock(&mMutex);

    pthread_mutex_unlock(&mMutex);
    */
    return;
}

/*===========================================================================
 * FUNCTION   : captureResultCb
 *
 * DESCRIPTION: Callback handler for all capture result (streams, as well as metadata)
 *
 * PARAMETERS :
 *   @metadata : metadata information
 *   @buffer   : actual gralloc buffer to be returned to frameworks. NULL if metadata.
 *
 * RETURN     : NONE
 *==========================================================================*/
void QCamera3HardwareInterface::captureResultCb(metadata_buffer_t *metadata,
                camera3_stream_buffer_t *buffer, uint32_t frame_number)
{
    pthread_mutex_lock(&mCaptureResultLock);
    camera3_capture_result_t result;


    if (metadata) {
        // Signal to unblock processCaptureRequest
        pthread_mutex_lock(&mRequestLock);
        mPendingRequest = 0;
        pthread_cond_signal(&mRequestCond);
        pthread_mutex_unlock(&mRequestLock);

        //TODO: Add translation from metadata_buffer_t to CameraMetadata
        // for now, hardcode timestamp only.
        CameraMetadata camMetadata;
        uint32_t *frame_number = (uint32_t *)POINTER_OF(CAM_INTF_META_FRAME_NUMBER, metadata);
        nsecs_t captureTime = 1000000 * (*frame_number) * 33;
        camMetadata.update(ANDROID_SENSOR_TIMESTAMP, &captureTime, 1);

        result.result = camMetadata.release();
        if (!result.result) {
            result.frame_number = *frame_number;
            result.num_output_buffers = 0;
            result.output_buffers = NULL;
            mCallbackOps->process_capture_result(mCallbackOps, &result);

            free_camera_metadata((camera_metadata_t*)result.result);
        }
    } else {
        result.result = NULL;
        result.frame_number = frame_number;
        result.num_output_buffers = 1;
        result.output_buffers = buffer;
        mCallbackOps->process_capture_result(mCallbackOps, &result);
    }

    pthread_mutex_unlock(&mCaptureResultLock);
    return;
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

    capabilityHeap = new QCamera3HeapMemory();
    if (capabilityHeap == NULL) {
        ALOGE("%s: creation of capabilityHeap failed", __func__);
        goto heap_creation_failed;
    }
    /* Allocate memory for capability buffer */
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
allocate_failed:
    delete capabilityHeap;
heap_creation_failed:
    cameraHandle->ops->close_camera(cameraHandle->camera_handle);
    cameraHandle = NULL;
open_failed:
    return rc;
}

/*===========================================================================
 * FUNCTION   : initParameters
 *
 * DESCRIPTION: initialize camera parameters
 *
 * PARAMETERS :
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::initParameters()
{
    int rc = 0;

    //Allocate Set Param Buffer
    mParamHeap = new QCamera3HeapMemory();
    rc = mParamHeap->allocate(1, sizeof(parm_buffer_t), false);
    if(rc != OK) {
        rc = NO_MEMORY;
        ALOGE("Failed to allocate SETPARM Heap memory");
        delete mParamHeap;
        mParamHeap = NULL;
        return rc;
    }

    //Map memory for parameters buffer
    rc = mCameraHandle->ops->map_buf(mCameraHandle->camera_handle,
            CAM_MAPPING_BUF_TYPE_PARM_BUF,
            mParamHeap->getFd(0),
            sizeof(parm_buffer_t));
    if(rc < 0) {
        ALOGE("%s:failed to map SETPARM buffer",__func__);
        rc = FAILED_TRANSACTION;
        mParamHeap->deallocate();
        delete mParamHeap;
        mParamHeap = NULL;
        return rc;
    }

    mParameters = (parm_buffer_t*) DATA_PTR(mParamHeap,0);
    memset(mParameters, 0, sizeof(parm_buffer_t));
    mParameters->first_flagged_entry = CAM_INTF_PARM_MAX;
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
    int facingBack = gCamCapability[cameraId]->position == CAM_POSITION_BACK;
    /*HAL 3 only*/
    #ifdef HAL_3_CAPABILITIES
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

    static int32_t scalar_formats[CAM_FORMAT_MAX];
    for (int i = 0; i < gCamCapability[cameraId]->supported_scalar_format_cnt; i++) {
        scalar_formats[i] = getScalarFormat(gCamCapability[cameraId]->supported_scalar_fmts[i]);
    }
    staticInfo.update(ANDROID_SCALER_AVAILABLE_FORMATS,
                      scalar_formats,
                      sizeof(scalar_formats)/sizeof(int32_t));

    static int32_t available_processed_sizes[CAM_FORMAT_MAX];
    makeTable(gCamCapability[cameraId]->supported_sizes_tbl,
              gCamCapability[cameraId]->supported_sizes_tbl_cnt,
              available_processed_sizes);
    staticInfo.update(ANDROID_SCALER_AVAILABLE_PROCESSED_SIZES,
                available_processed_sizes,
                sizeof(available_processed_sizes)/sizeof(int32_t));
    #else
    const float minFocusDistance = 0;
    staticInfo.update(ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE,
                    &minFocusDistance, 1);

    const float hyperFocusDistance = 0;
    staticInfo.update(ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE,
                     &hyperFocusDistance, 1);

    static const float focalLength = 3.30f;
    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS,
                      &focalLength,
                      1);

    static const float aperture = 2.8f;
    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_APERTURES,
                      &aperture,
                      1);

    static const float filterDensity = 0;
    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES,
                      &filterDensity, 1);

    static const uint8_t availableOpticalStabilization =
            ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF;
    staticInfo.update(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION,
                      &availableOpticalStabilization, 1);

    float lensPosition[3];
    if (facingBack) {
        // Back-facing camera is center-top on device
        lensPosition[0] = 0;
        lensPosition[1] = 20;
        lensPosition[2] = -5;
    } else {
        // Front-facing camera is center-right on device
        lensPosition[0] = 20;
        lensPosition[1] = 20;
        lensPosition[2] = 0;
    }
    staticInfo.update(ANDROID_LENS_POSITION,
                      lensPosition,
                      sizeof(lensPosition)/ sizeof(float));

    static const int32_t lensShadingMapSize[] = {1, 1};
    staticInfo.update(ANDROID_LENS_INFO_SHADING_MAP_SIZE,
                      lensShadingMapSize,
                      sizeof(lensShadingMapSize)/sizeof(int32_t));

    static const float lensShadingMap[3 * 1 * 1 ] =
            { 1.f, 1.f, 1.f };
    staticInfo.update(ANDROID_LENS_INFO_SHADING_MAP,
                      lensShadingMap,
                      sizeof(lensShadingMap)/ sizeof(float));

    static const int32_t geometricCorrectionMapSize[] = {2, 2};
    staticInfo.update(ANDROID_LENS_INFO_GEOMETRIC_CORRECTION_MAP_SIZE,
                      geometricCorrectionMapSize,
                      sizeof(geometricCorrectionMapSize)/sizeof(int32_t));

    static const float geometricCorrectionMap[2 * 3 * 2 * 2] = {
            0.f, 0.f,  0.f, 0.f,  0.f, 0.f,
            1.f, 0.f,  1.f, 0.f,  1.f, 0.f,
            0.f, 1.f,  0.f, 1.f,  0.f, 1.f,
            1.f, 1.f,  1.f, 1.f,  1.f, 1.f};
    staticInfo.update(ANDROID_LENS_INFO_GEOMETRIC_CORRECTION_MAP,
                      geometricCorrectionMap,
                      sizeof(geometricCorrectionMap)/ sizeof(float));

    static const float sensorPhysicalSize[2] = {3.20f, 2.40f};
    staticInfo.update(ANDROID_SENSOR_INFO_PHYSICAL_SIZE,
                      sensorPhysicalSize, 2);

    const int64_t exposureTimeRange[2] = {1000L, 30000000000L} ; // 1 us - 30 sec
    staticInfo.update(ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE,
                      exposureTimeRange, 2);

    const int64_t frameDurationRange[2] = {33331760L, 30000000000L};
    staticInfo.update(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION,
                      frameDurationRange, 1);

    const uint8_t colorFilterArrangement =
                         ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB;
    staticInfo.update(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT,
                      &colorFilterArrangement, 1);

    const int resolution[2]  = {640, 480};
    staticInfo.update(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE,
                      resolution, 2);

    staticInfo.update(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE,
                      resolution, 2);

    const uint32_t whiteLevel = 4000;
    staticInfo.update(ANDROID_SENSOR_INFO_WHITE_LEVEL,
                      (int32_t*)&whiteLevel, 1);

    static const int32_t blackLevelPattern[4] = {
            1000, 1000,
            1000, 1000 };
    staticInfo.update(ANDROID_SENSOR_BLACK_LEVEL_PATTERN,
                      blackLevelPattern, 4);

    static const int64_t flashChargeDuration = 0;
    staticInfo.update(ANDROID_FLASH_INFO_CHARGE_DURATION,
                      &flashChargeDuration, 1);

    static const int32_t tonemapCurvePoints = 128;
    staticInfo.update(ANDROID_TONEMAP_MAX_CURVE_POINTS,
                      &tonemapCurvePoints, 1);

    static const int32_t maxFaceCount = 8;
    staticInfo.update(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT,
                      &maxFaceCount, 1);

    static const int32_t histogramSize = 64;
    staticInfo.update(ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT,
                      &histogramSize, 1);

    static const int32_t maxHistogramCount = 1000;
    staticInfo.update(ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT,
                      &maxHistogramCount, 1);

    static const int32_t sharpnessMapSize[2] = {64, 64};
    staticInfo.update(ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE,
                      sharpnessMapSize, sizeof(sharpnessMapSize)/sizeof(int32_t));

    static const int32_t maxSharpnessMapValue = 1000;
    staticInfo.update(ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE,
                      &maxSharpnessMapValue, 1);

    static const uint8_t availableVstabModes[] = {ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF};
    staticInfo.update(ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES,
                      availableVstabModes, sizeof(availableVstabModes));

    const uint64_t availableRawMinDurations[1] = {33331760L};
    staticInfo.update(ANDROID_SCALER_AVAILABLE_RAW_MIN_DURATIONS,
                      (int64_t*)&availableRawMinDurations,
                       1);

    const uint32_t availableFormats[4] = {
        HAL_PIXEL_FORMAT_RAW_SENSOR,
        HAL_PIXEL_FORMAT_BLOB,
        HAL_PIXEL_FORMAT_RGBA_8888,
        HAL_PIXEL_FORMAT_YCrCb_420_SP
    };
    staticInfo.update(ANDROID_SCALER_AVAILABLE_FORMATS,
                      (int32_t*)availableFormats,
                      4);

    const uint32_t availableProcessedSizes[4] = {640, 480, 320, 240};
    staticInfo.update(ANDROID_SCALER_AVAILABLE_PROCESSED_SIZES,
                      (int32_t*)availableProcessedSizes,
                      sizeof(availableProcessedSizes)/sizeof(int32_t));

    staticInfo.update(ANDROID_SCALER_AVAILABLE_JPEG_SIZES,
                      resolution,
                     sizeof(resolution)/sizeof(int));

    static const uint8_t availableFaceDetectModes[] = {
        ANDROID_STATISTICS_FACE_DETECT_MODE_OFF };

    staticInfo.update(ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES,
                      availableFaceDetectModes,
                      sizeof(availableFaceDetectModes));

    static const uint8_t availableSceneModes[] = {
            ANDROID_CONTROL_SCENE_MODE_UNSUPPORTED };

    staticInfo.update(ANDROID_CONTROL_AVAILABLE_SCENE_MODES,
            availableSceneModes, sizeof(availableSceneModes));

    static const int32_t availableFpsRanges[] = {15, 30};
    staticInfo.update(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES,
            availableFpsRanges, sizeof(availableFpsRanges)/sizeof(int32_t));

    static const uint8_t availableEffectsModes[] = {
            ANDROID_CONTROL_EFFECT_MODE_OFF };
    staticInfo.update(ANDROID_CONTROL_AVAILABLE_EFFECTS,
            availableEffectsModes, sizeof(availableEffectsModes));

    static const uint8_t availableAntibandingModes[] = {
            ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF };
    staticInfo.update(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
            availableAntibandingModes, sizeof(availableAntibandingModes));

    static const uint8_t flashAvailable = 0;
    staticInfo.update(ANDROID_FLASH_INFO_AVAILABLE,
            &flashAvailable, sizeof(flashAvailable));

    static const int32_t max3aRegions = 0;
    staticInfo.update(ANDROID_CONTROL_MAX_REGIONS,
            &max3aRegions, 1);

    static const camera_metadata_rational exposureCompensationStep = {
            1, 3
    };
    staticInfo.update(ANDROID_CONTROL_AE_COMPENSATION_STEP,
            &exposureCompensationStep, 1);

    static const int32_t jpegThumbnailSizes[] = {
            0, 0,
            160, 120,
            320, 240
     };
    staticInfo.update(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES,
            jpegThumbnailSizes, sizeof(jpegThumbnailSizes)/sizeof(int32_t));

    static const int32_t maxZoom = 10;
    staticInfo.update(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM,
            &maxZoom, 1);

    static int64_t jpegMinDuration[] = {33331760L, 30000000000L};
    staticInfo.update(ANDROID_SCALER_AVAILABLE_JPEG_MIN_DURATIONS,
                      jpegMinDuration,
                      sizeof(jpegMinDuration)/sizeof(uint64_t));
    #endif
     /*HAL 1 and HAL 3 common*/
     static const int32_t raw_size[] = {gCamCapability[cameraId]->raw_dim.width,
                                       gCamCapability[cameraId]->raw_dim.height};
    staticInfo.update(ANDROID_SCALER_AVAILABLE_RAW_SIZES,
                      raw_size,
                      sizeof(raw_size)/sizeof(uint32_t));

    static const int32_t exposureCompensationRange[] = {gCamCapability[cameraId]->exposure_compensation_min,
                                                        gCamCapability[cameraId]->exposure_compensation_max};
    staticInfo.update(ANDROID_CONTROL_AE_COMPENSATION_RANGE,
            exposureCompensationRange,
            sizeof(exposureCompensationRange)/sizeof(int32_t));

    uint8_t lensFacing = (facingBack) ?
            ANDROID_LENS_FACING_BACK : ANDROID_LENS_FACING_FRONT;
    staticInfo.update(ANDROID_LENS_FACING, &lensFacing, 1);

    static int32_t available_jpeg_sizes[MAX_SIZES_CNT];
    makeTable(gCamCapability[cameraId]->picture_sizes_tbl,
              gCamCapability[cameraId]->picture_sizes_tbl_cnt,
              available_jpeg_sizes);
    staticInfo.update(ANDROID_SCALER_AVAILABLE_JPEG_SIZES,
                available_jpeg_sizes,
                sizeof(available_jpeg_sizes)/sizeof(int32_t));

    static int32_t avail_effects[CAM_EFFECT_MODE_MAX];
    for (int i = 0; i < gCamCapability[cameraId]->supported_effects_cnt; i++) {
        avail_effects[i] = lookupFwkName(EFFECT_MODES_MAP,
                                         sizeof(EFFECT_MODES_MAP)/sizeof(int),
                                         gCamCapability[cameraId]->supported_effects[i]);
    }
    staticInfo.update(ANDROID_CONTROL_AVAILABLE_EFFECTS,
                      avail_effects,
                      sizeof(avail_effects)/sizeof(int32_t));

    static int32_t avail_scene_modes[CAM_SCENE_MODE_MAX];
    for (int i = 0; i < gCamCapability[cameraId]->supported_scene_modes_cnt; i++) {
        avail_scene_modes[i] = lookupFwkName(SCENE_MODES_MAP,
                                         sizeof(SCENE_MODES_MAP)/sizeof(int),
                                         gCamCapability[cameraId]->supported_scene_modes[i]);
    }
    staticInfo.update(ANDROID_CONTROL_AVAILABLE_SCENE_MODES,
                      avail_scene_modes,
                      sizeof(avail_scene_modes)/sizeof(int32_t));

    static int32_t avail_antibanding_modes[CAM_ANTIBANDING_MODE_MAX];
    for (int i = 0; i < gCamCapability[cameraId]->supported_antibandings_cnt; i++) {
        avail_antibanding_modes[i] = lookupFwkName(ANTIBANDING_MODES_MAP,
                                         sizeof(ANTIBANDING_MODES_MAP)/sizeof(int),
                                         gCamCapability[cameraId]->supported_antibandings[i]);
    }
    staticInfo.update(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES,
                      avail_antibanding_modes,
                      sizeof(avail_antibanding_modes)/sizeof(int32_t));

    static uint8_t avail_af_modes[CAM_FOCUS_MODE_MAX];
    for (int i = 0; i < gCamCapability[cameraId]->supported_focus_modes_cnt; i++) {
        avail_af_modes[i] = lookupFwkName(FOCUS_MODES_MAP,
                                         sizeof(FOCUS_MODES_MAP)/sizeof(int),
                                         gCamCapability[cameraId]->supported_focus_modes[i]);
    }
    staticInfo.update(ANDROID_CONTROL_AF_AVAILABLE_MODES,
                      avail_af_modes,
                      sizeof(avail_af_modes));

    static int32_t avail_awb_modes[CAM_WB_MODE_MAX];
    for (int i = 0; i < gCamCapability[cameraId]->supported_white_balances_cnt; i++) {
        avail_awb_modes[i] = lookupFwkName(WHITE_BALANCE_MODES_MAP,
                                         sizeof(WHITE_BALANCE_MODES_MAP)/sizeof(int),
                                         gCamCapability[cameraId]->supported_white_balances[i]);
    }
    staticInfo.update(ANDROID_CONTROL_AWB_AVAILABLE_MODES,
                      avail_awb_modes,
                      sizeof(avail_awb_modes)/sizeof(int32_t));

    static int32_t avail_flash_modes[CAM_FLASH_MODE_MAX];
    for (int i = 0; i < gCamCapability[cameraId]->supported_flash_modes_cnt; i++) {
        avail_flash_modes[i] = lookupFwkName(FLASH_MODES_MAP,
                                         sizeof(FLASH_MODES_MAP)/sizeof(int),
                                         gCamCapability[cameraId]->supported_flash_modes[i]);
    }
    staticInfo.update(ANDROID_FLASH_MODE,
                      avail_flash_modes,
                      sizeof(avail_flash_modes)/sizeof(int32_t));

    /*so far fwk seems to support only 2 aec modes on and off*/
    static const uint8_t avail_ae_modes[] = {
            ANDROID_CONTROL_AE_MODE_OFF,
            ANDROID_CONTROL_AE_MODE_ON
    };
    staticInfo.update(ANDROID_CONTROL_AE_AVAILABLE_MODES,
                      avail_ae_modes,
                      sizeof(avail_ae_modes));

    gStaticMetadata = staticInfo.release();
    return rc;
}

/*===========================================================================
 * FUNCTION   : makeTable
 *
 * DESCRIPTION: make a table of sizes
 *
 * PARAMETERS :
 *
 *
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
void QCamera3HardwareInterface::makeTable(cam_dimension_t* dimTable, uint8_t size,
                                          int32_t* sizeTable)
{
    int j = 0;
    for (int i = 0; i < size; i++) {
        sizeTable[j] = dimTable[i].width;
        sizeTable[j+1] = dimTable[i].height;
        j+=2;
    }
}
/*===========================================================================
 * FUNCTION   : getPreviewHalPixelFormat
 *
 * DESCRIPTION: convert the format to type recognized by framework
 *
 * PARAMETERS : format : the format from backend
 *
 ** RETURN    : format recognized by framework
 *
 *==========================================================================*/
int32_t QCamera3HardwareInterface::getScalarFormat(int32_t format)
{
    int32_t halPixelFormat;

    switch (format) {
    case CAM_FORMAT_YUV_420_NV12:
        halPixelFormat = HAL_PIXEL_FORMAT_YCbCr_420_SP;
        break;
    case CAM_FORMAT_YUV_420_NV21:
        halPixelFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        break;
    case CAM_FORMAT_YUV_420_NV21_ADRENO:
        halPixelFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP_ADRENO;
        break;
    case CAM_FORMAT_YUV_420_YV12:
        halPixelFormat = HAL_PIXEL_FORMAT_YV12;
        break;
    case CAM_FORMAT_YUV_422_NV16:
    case CAM_FORMAT_YUV_422_NV61:
    default:
        halPixelFormat = HAL_PIXEL_FORMAT_YCrCb_420_SP;
        break;
    }
    return halPixelFormat;
}

/*===========================================================================
 * FUNCTION   : AddSetParmEntryToBatch
 *
 * DESCRIPTION: add set parameter entry into batch
 *
 * PARAMETERS :
 *   @p_table     : ptr to parameter buffer
 *   @paramType   : parameter type
 *   @paramLength : length of parameter value
 *   @paramValue  : ptr to parameter value
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3HardwareInterface::AddSetParmEntryToBatch(parm_buffer_t *p_table,
                                                          cam_intf_parm_type_t paramType,
                                                          uint32_t paramLength,
                                                          void *paramValue)
{
    int position = paramType;
    int current, next;

    /*************************************************************************
    *                 Code to take care of linking next flags                *
    *************************************************************************/
    current = GET_FIRST_PARAM_ID(p_table);
    if (position == current){
        //DO NOTHING
    } else if (position < current){
        SET_NEXT_PARAM_ID(position, p_table, current);
        SET_FIRST_PARAM_ID(p_table, position);
    } else {
        /* Search for the position in the linked list where we need to slot in*/
        while (position > GET_NEXT_PARAM_ID(current, p_table))
            current = GET_NEXT_PARAM_ID(current, p_table);

        /*If node already exists no need to alter linking*/
        if (position != GET_NEXT_PARAM_ID(current, p_table)) {
            next = GET_NEXT_PARAM_ID(current, p_table);
            SET_NEXT_PARAM_ID(current, p_table, position);
            SET_NEXT_PARAM_ID(position, p_table, next);
        }
    }

    /*************************************************************************
    *                   Copy contents into entry                             *
    *************************************************************************/

    if (paramLength > sizeof(parm_type_t)) {
        ALOGE("%s:Size of input larger than max entry size",__func__);
        return BAD_VALUE;
    }
    memcpy(POINTER_OF(paramType,p_table), paramValue, paramLength);
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : lookupFwkName
 *
 * DESCRIPTION: In case the enum is not same in fwk and backend
 *              make sure the parameter is correctly propogated
 *
 * PARAMETERS  :
 *   @arr      : map between the two enums
 *   @len      : len of the map
 *   @hal_name : name of the hal_parm to map
 *
 * RETURN     : int type of status
 *              fwk_name  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::lookupFwkName(const QCameraMap arr[],
                                             int len, int hal_name)
{

   for (int i = 0; i < len; i++) {
       if (arr[i].hal_name == hal_name)
           return arr[i].fwk_name;
    }
    return NAME_NOT_FOUND;
}

/*===========================================================================
 * FUNCTION   : lookupHalName
 *
 * DESCRIPTION: In case the enum is not same in fwk and backend
 *              make sure the parameter is correctly propogated
 *
 * PARAMETERS  :
 *   @arr      : map between the two enums
 *   @len      : len of the map
 *   @fwk_name : name of the hal_parm to map
 *
 * RETURN     : int32_t type of status
 *              hal_name  -- success
 *              none-zero failure code
 *==========================================================================*/
int QCamera3HardwareInterface::lookupHalName(const QCameraMap arr[],
                                             int len, int fwk_name)
{
    for (int i = 0; i < len; i++) {
       if (arr[i].fwk_name == fwk_name)
           return arr[i].hal_name;
    }
    return NAME_NOT_FOUND;
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
    info->device_version = HARDWARE_DEVICE_API_VERSION(3, 0);
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
camera_metadata_t* QCamera3HardwareInterface::translateToMetadata(int type)
{
    pthread_mutex_lock(&mMutex);

    if (mDefaultMetadata[type] != NULL) {
        pthread_mutex_unlock(&mMutex);
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

    settings.update(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
            &gCamCapability[mCameraId]->exposure_compensation_default, 1);

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

    pthread_mutex_unlock(&mMutex);
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
int QCamera3HardwareInterface::setFrameParameters(int frame_id,
                                                  const camera_metadata_t *settings)
{
    /*translate from camera_metadata_t type to parm_type_t*/
    int rc = 0;
    if (settings == NULL && mParameters == NULL) {
        /*settings cannot be null for the first request*/
        return BAD_VALUE;
    }
    /*we need to update the frame number in the parameters*/
    rc = AddSetParmEntryToBatch(mParameters, CAM_INTF_META_FRAME_NUMBER,
                                sizeof(frame_id), &frame_id);
    if (rc < 0) {
        ALOGE("%s: Failed to set the frame number in the parameters", __func__);
        return BAD_VALUE;
    }
    if(settings != NULL){
        rc = translateMetadataToParameters(settings);
    }
    /*set the parameters to backend*/
    mCameraHandle->ops->set_parms(mCameraHandle->camera_handle, mParameters);
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
    int rc = 0;
    android::CameraMetadata frame_settings;
    frame_settings = settings;

    //white balance
    int32_t fwk_whiteLevel = frame_settings.find(ANDROID_CONTROL_AWB_MODE).data.i32[0];
    int whiteLevel = lookupHalName(WHITE_BALANCE_MODES_MAP,
                                   sizeof(WHITE_BALANCE_MODES_MAP)/ sizeof(int32_t),
                                   fwk_whiteLevel);
    rc = AddSetParmEntryToBatch(mParameters, CAM_INTF_PARM_WHITE_BALANCE,
                                sizeof(whiteLevel), &whiteLevel);
    //effect
    int32_t fwk_effectMode = frame_settings.find(ANDROID_CONTROL_EFFECT_MODE).data.i32[0];
    int effectMode = lookupHalName(EFFECT_MODES_MAP,
                                   sizeof(EFFECT_MODES_MAP)/ sizeof(int32_t),
                                   fwk_effectMode);
    rc = AddSetParmEntryToBatch(mParameters, CAM_INTF_PARM_EFFECT,
                                sizeof(effectMode), &effectMode);
    //ae mode
    int32_t fwk_aeMode = frame_settings.find(ANDROID_CONTROL_AE_MODE).data.i32[0];
    int aeMode = lookupHalName(AUTO_EXPOSURE_MAP,
                               sizeof(AUTO_EXPOSURE_MAP)/ sizeof(int32_t),
                               fwk_aeMode);
    rc = AddSetParmEntryToBatch(mParameters, CAM_INTF_META_AEC_MODE,
                                sizeof(aeMode), &aeMode);

    //scaler crop region
    int32_t scalerCropRegion = frame_settings.find(ANDROID_SCALER_CROP_REGION).data.i32[0];
    rc = AddSetParmEntryToBatch(mParameters, CAM_INTF_META_SCALER_CROP_REGION,
                                sizeof(scalerCropRegion), &scalerCropRegion);

    //capture_intent
    int32_t captureIntent = frame_settings.find(ANDROID_CONTROL_CAPTURE_INTENT).data.i32[0];
    rc = AddSetParmEntryToBatch(mParameters, CAM_INTF_META_CAPTURE_INTENT,
                                sizeof(captureIntent), &captureIntent);

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
int QCamera3HardwareInterface::getJpegSettings
                                  (const camera_metadata_t *settings)
{
    if (mJpegSettings) {
        free(mJpegSettings);
        mJpegSettings = NULL;
    }
    mJpegSettings = (jpeg_settings_t*) malloc(sizeof(jpeg_settings_t));
    android::CameraMetadata jpeg_settings;
    jpeg_settings = settings;

    mJpegSettings->jpeg_orientation =
        jpeg_settings.find(ANDROID_JPEG_ORIENTATION).data.i32[0];
    mJpegSettings->jpeg_quality =
        jpeg_settings.find(ANDROID_JPEG_QUALITY).data.u8[0];
    mJpegSettings->thumbnail_size.height =
        jpeg_settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32[0];
    mJpegSettings->thumbnail_size.height =
        jpeg_settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.i32[1];
    for (int i = 0; i < 3; i++) {
        mJpegSettings->gps_coordinates[i] =
            jpeg_settings.find(ANDROID_JPEG_THUMBNAIL_SIZE).data.d[i];
    }
    mJpegSettings->gps_timestamp =
        jpeg_settings.find(ANDROID_JPEG_GPS_TIMESTAMP).data.i64[0];
    mJpegSettings->gps_processing_method =
        jpeg_settings.find(ANDROID_JPEG_GPS_PROCESSING_METHOD).data.u8[0];
    mJpegSettings->sensor_sensitivity =
        jpeg_settings.find(ANDROID_SENSOR_SENSITIVITY).data.i32[0];
    mJpegSettings->lens_focal_length =
        jpeg_settings.find(ANDROID_LENS_FOCAL_LENGTH).data.f[0];
    return 0;
}

/*===========================================================================
 * FUNCTION   : captureResultCb
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
void QCamera3HardwareInterface::captureResultCb(metadata_buffer_t *metadata,
                camera3_stream_buffer_t *buffer,
                uint32_t frame_number, void *userdata)
{
    QCamera3HardwareInterface *hw = (QCamera3HardwareInterface *)userdata;
    if (hw == NULL) {
        ALOGE("%s: Invalid hw %p", __func__, hw);
        return;
    }

    hw->captureResultCb(metadata, buffer, frame_number);
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
const camera_metadata_t* QCamera3HardwareInterface::
    construct_default_request_settings(const struct camera3_device *device,
                                        int type)
{

    camera_metadata_t* fwk_metadata = NULL;
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return NULL;
    }

    fwk_metadata = hw->translateToMetadata(type);

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
int QCamera3HardwareInterface::process_capture_request(
                    const struct camera3_device *device,
                    camera3_capture_request_t *request)
{
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return -EINVAL;
    }

    return hw->processCaptureRequest(request);
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

void QCamera3HardwareInterface::get_metadata_vendor_tag_ops(
                const struct camera3_device *device,
                vendor_tag_query_ops_t* ops)
{
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return;
    }

    hw->getMetadataVendorTagOps(ops);
    return;
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

void QCamera3HardwareInterface::dump(
                const struct camera3_device *device, int fd)
{
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(device->priv);
    if (!hw) {
        ALOGE("%s: NULL camera device", __func__);
        return;
    }

    hw->dump(fd);
    return;
}

/*===========================================================================
 * FUNCTION   : close_camera_device
 *
 * DESCRIPTION:
 *
 * PARAMETERS :
 *
 *
 * RETURN     :
 *==========================================================================*/
int QCamera3HardwareInterface::close_camera_device(struct hw_device_t* device)
{
    int ret = NO_ERROR;
    QCamera3HardwareInterface *hw =
        reinterpret_cast<QCamera3HardwareInterface *>(
            reinterpret_cast<camera3_device_t *>(device)->priv);
    if (!hw) {
        ALOGE("NULL camera device");
        return BAD_VALUE;
    }
    delete hw;
    return ret;
}


}; //end namespace qcamera
