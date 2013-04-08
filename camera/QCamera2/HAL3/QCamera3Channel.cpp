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

#define LOG_TAG "QCamera3Channel"

#include <hardware/camera3.h>
#include <gralloc_priv.h>
#include <utils/Log.h>
#include <utils/Errors.h>
#include "QCamera3Channel.h"

using namespace android;

namespace qcamera {

/*===========================================================================
 * FUNCTION   : QCamera3Channel
 *
 * DESCRIPTION: constrcutor of QCamera3Channel
 *
 * PARAMETERS :
 *   @cam_handle : camera handle
 *   @cam_ops    : ptr to camera ops table
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3Channel::QCamera3Channel(uint32_t cam_handle,
                               mm_camera_ops_t *cam_ops,
                               channel_cb_routine cb_routine)
{
    m_camHandle = cam_handle;
    m_camOps = cam_ops;
    mChannelCB = cb_routine;
    m_bIsActive = false;

    m_handle = 0;
    m_numStreams = 0;
    memset(mStreams, 0, sizeof(mStreams));
}

/*===========================================================================
 * FUNCTION   : QCamera3Channel
 *
 * DESCRIPTION: default constrcutor of QCamera3Channel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3Channel::QCamera3Channel()
{
    m_camHandle = 0;
    m_camOps = NULL;
    m_bIsActive = false;

    m_handle = 0;
    m_numStreams = 0;
    memset(mStreams, 0, sizeof(mStreams));
}

/*===========================================================================
 * FUNCTION   : ~QCamera3Channel
 *
 * DESCRIPTION: destructor of QCamera3Channel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3Channel::~QCamera3Channel()
{
    if (m_bIsActive) {
        stop();
    }

    for (int i = 0; i < m_numStreams; i++) {
        if (mStreams[i] != NULL) {
            delete mStreams[i];
            mStreams[i] = 0;
        }
    }
    m_numStreams = 0;
    m_camOps->delete_channel(m_camHandle, m_handle);
    m_handle = 0;
}

/*===========================================================================
 * FUNCTION   : init
 *
 * DESCRIPTION: initialization of channel
 *
 * PARAMETERS :
 *   @attr    : channel bundle attribute setting
 *   @dataCB  : data notify callback
 *   @userData: user data ptr
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3Channel::init(mm_camera_channel_attr_t *attr,
                             mm_camera_buf_notify_t dataCB,
                             cam_padding_info_t *paddingInfo,
                             void *userData)
{
    m_handle = m_camOps->add_channel(m_camHandle,
                                      attr,
                                      dataCB,
                                      userData);
    if (m_handle == 0) {
        ALOGE("%s: Add channel failed", __func__);
        return UNKNOWN_ERROR;
    }
    mPaddingInfo = paddingInfo;
    return NO_ERROR;
}

/*===========================================================================
 * FUNCTION   : addStream
 *
 * DESCRIPTION: add a stream into channel
 *
 * PARAMETERS :
 *   @allocator      : stream related buffer allocator
 *   @streamInfoBuf  : ptr to buf that constains stream info
 *   @minStreamBufNum: number of stream buffers needed
 *   @paddingInfo    : padding information
 *   @stream_cb      : stream data notify callback
 *   @userdata       : user data ptr
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3Channel::addStream(cam_stream_type_t streamType,
                                  cam_format_t streamFormat,
                                  cam_dimension_t streamDim,
                                  uint8_t minStreamBufNum,
                                  cam_padding_info_t *paddingInfo,
                                  QCamera3Memory *memory)
{
    int32_t rc = NO_ERROR;
    if (m_numStreams >= MAX_STREAM_NUM_IN_BUNDLE) {
        ALOGE("%s: stream number (%d) exceeds max limit (%d)",
              __func__, m_numStreams, MAX_STREAM_NUM_IN_BUNDLE);
        return BAD_VALUE;
    }
    QCamera3Stream *pStream = new QCamera3Stream(m_camHandle,
                                               m_handle,
                                               m_camOps,
                                               paddingInfo,
                                               memory);
    if (pStream == NULL) {
        ALOGE("%s: No mem for Stream", __func__);
        return NO_MEMORY;
    }

    rc = pStream->init(streamType, streamFormat, streamDim, minStreamBufNum, streamCbRoutine, this);
    if (rc == 0) {
        mStreams[m_numStreams] = pStream;
        m_numStreams++;
    } else {
        delete pStream;
    }
    return rc;
}

/*===========================================================================
 * FUNCTION   : start
 *
 * DESCRIPTION: start channel, which will start all streams belong to this channel
 *
 * PARAMETERS :
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3Channel::start()
{
    int32_t rc = NO_ERROR;

    if (m_numStreams > 1) {
        ALOGE("%s: bundle not supported", __func__);
    }

    for (int i = 0; i < m_numStreams; i++) {
        if (mStreams[i] != NULL) {
            mStreams[i]->start();
        }
    }
    rc = m_camOps->start_channel(m_camHandle, m_handle);

    if (rc != NO_ERROR) {
        for (int i = 0; i < m_numStreams; i++) {
            if (mStreams[i] != NULL) {
                mStreams[i]->stop();
            }
        }
    } else {
        m_bIsActive = true;
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : stop
 *
 * DESCRIPTION: stop a channel, which will stop all streams belong to this channel
 *
 * PARAMETERS : none
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3Channel::stop()
{
    int32_t rc = NO_ERROR;
    rc = m_camOps->stop_channel(m_camHandle, m_handle);

    for (int i = 0; i < m_numStreams; i++) {
        if (mStreams[i] != NULL) {
            mStreams[i]->stop();
        }
    }

    m_bIsActive = false;
    return rc;
}

/*===========================================================================
 * FUNCTION   : bufDone
 *
 * DESCRIPTION: return a stream buf back to kernel
 *
 * PARAMETERS :
 *   @recvd_frame  : stream buf frame to be returned
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCamera3Channel::bufDone(mm_camera_super_buf_t *recvd_frame)
{
    int32_t rc = NO_ERROR;
    for (int i = 0; i < recvd_frame->num_bufs; i++) {
         if (recvd_frame->bufs[i] != NULL) {
             for (int j = 0; j < m_numStreams; j++) {
                 if (mStreams[j] != NULL &&
                     mStreams[j]->getMyHandle() == recvd_frame->bufs[i]->stream_id) {
                     rc = mStreams[j]->bufDone(recvd_frame->bufs[i]->buf_idx);
                     break; // break loop j
                 }
             }
         }
    }

    return rc;
}

/*===========================================================================
 * FUNCTION   : getStreamByHandle
 *
 * DESCRIPTION: return stream object by stream handle
 *
 * PARAMETERS :
 *   @streamHandle : stream handle
 *
 * RETURN     : stream object. NULL if not found
 *==========================================================================*/
QCamera3Stream *QCamera3Channel::getStreamByHandle(uint32_t streamHandle)
{
    for (int i = 0; i < m_numStreams; i++) {
        if (mStreams[i] != NULL && mStreams[i]->getMyHandle() == streamHandle) {
            return mStreams[i];
        }
    }
    return NULL;
}

/*===========================================================================
 * FUNCTION   : getStreamByHandle
 *
 * DESCRIPTION: return stream object by stream handle
 *
 * PARAMETERS :
 *   @streamHandle : stream handle
 *
 * RETURN     : stream object. NULL if not found
 *==========================================================================*/
QCamera3Stream *QCamera3Channel::getStreamByIndex(uint8_t index)
{
    if (index < m_numStreams) {
        return mStreams[index];
    }
    return NULL;
}

/*===========================================================================
 * FUNCTION   : streamCbRoutine
 *
 * DESCRIPTION: callback routine for stream
 *
 * PARAMETERS :
 *   @streamHandle : stream handle
 *
 * RETURN     : stream object. NULL if not found
 *==========================================================================*/
void QCamera3Channel::streamCbRoutine(mm_camera_super_buf_t *super_frame,
                QCamera3Stream *stream, void *userdata)
{
    QCamera3Channel *channel = (QCamera3Channel *)userdata;
    if (channel == NULL) {
        ALOGE("%s: invalid channel pointer", __func__);
        return;
    }
    channel->streamCbRoutine(super_frame, stream);
}

/*===========================================================================
 * FUNCTION   : QCamera3RegularChannel
 *
 * DESCRIPTION: constrcutor of QCamera3RegularChannel
 *
 * PARAMETERS :
 *   @cam_handle : camera handle
 *   @cam_ops    : ptr to camera ops table
 *   @cb_routine : callback routine to frame aggregator
 *   @stream     : camera3_stream_t structure
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3RegularChannel::QCamera3RegularChannel(uint32_t cam_handle,
                    mm_camera_ops_t *cam_ops,
                    channel_cb_routine cb_routine,
                    camera3_stream_t *stream) :
                        QCamera3Channel(cam_handle, cam_ops, cb_routine),
                        mCamera3Stream(stream),
                        mCamera3Buffers(NULL),
                        mMemory(NULL)
{
    //TODO
}

/*===========================================================================
 * FUNCTION   : ~QCamera3RegularChannel
 *
 * DESCRIPTION: destructor of QCamera3RegularChannel
 *
 * PARAMETERS : none
 *
 * RETURN     : none
 *==========================================================================*/
QCamera3RegularChannel::~QCamera3RegularChannel()
{
    //TODO
    if (mMemory) {
        delete mMemory;
    }
}

/*===========================================================================
 * FUNCTION   : request
 *
 * DESCRIPTION: process a request from camera service. Stream on if ncessary.
 *
 * PARAMETERS :
 *   @buffer  : buffer to be filled for this request
 *
 * RETURN     : 0 on a success start of capture
 *              -EINVAL on invalid input
 *              -ENODEV on serious error
 *==========================================================================*/
int32_t QCamera3RegularChannel::request(camera3_stream_buffer_t *buffer)
{
    //TODO
    return 0;
}

/*===========================================================================
 * FUNCTION   : registerBuffers
 *
 * DESCRIPTION: register streaming buffers to the channel object
 *
 * PARAMETERS :
 *   @num_buffers : number of buffers to be registered
 *   @buffers     : buffer to be registered
 *
 * RETURN     : 0 on a success start of capture
 *              -EINVAL on invalid input
 *              -ENOMEM on failure to register the buffer
 *              -ENODEV on serious error
 *==========================================================================*/
int32_t QCamera3RegularChannel::registerBuffers(uint32_t num_buffers, buffer_handle_t **buffers)
{
    int rc = 0;
    struct private_handle_t *priv_handle = (struct private_handle_t *)(*buffers[0]);
    cam_stream_type_t streamType;
    cam_format_t streamFormat;
    cam_dimension_t streamDim;

    if (mCamera3Stream->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED) {
        if (priv_handle->flags & private_handle_t::PRIV_FLAGS_VIDEO_ENCODER) {
            streamType = CAM_STREAM_TYPE_VIDEO;
            streamFormat = CAM_FORMAT_YUV_420_NV12;
        } else if (priv_handle->flags & private_handle_t::PRIV_FLAGS_EXTERNAL_ONLY) {
            streamType = CAM_STREAM_TYPE_PREVIEW;
            streamFormat = CAM_FORMAT_YUV_420_NV21;
        } else {
            ALOGE("%s: priv_handle->flags 0x%x not supported",
                    __func__, priv_handle->flags);
            return -EINVAL;
        }
    } else {
        //TODO: Fail for other types of streams for now
        ALOGE("%s: format is not IMPLEMENTATION_DEFINED", __func__);
        return -EINVAL;
    }

    mMemory = new QCamera3GrallocMemory();
    if (mMemory == NULL) {
        return -ENOMEM;
    }

    rc = mMemory->registerBuffers(num_buffers, buffers);
    if (rc < 0) {
        delete mMemory;
        return rc;
    }

    streamDim.width = mCamera3Stream->width;
    streamDim.height = mCamera3Stream->height;
    rc = QCamera3Channel::addStream(streamType, streamFormat, streamDim,
        num_buffers, mPaddingInfo, mMemory);

    return rc;
}

void QCamera3RegularChannel::streamCbRoutine(mm_camera_super_buf_t *super_frame,
                            QCamera3Stream *stream)
{
    //TODO
    return;
}

int QCamera3RegularChannel::kMaxBuffers = 3;

QCamera3MetadataChannel::QCamera3MetadataChannel(uint32_t cam_handle,
                    mm_camera_ops_t *cam_ops,
                    channel_cb_routine cb_routine) :
                        QCamera3Channel(cam_handle, cam_ops, cb_routine)
{
    //TODO
}

QCamera3MetadataChannel::~QCamera3MetadataChannel()
{
    //TODO
}

int32_t QCamera3MetadataChannel::initialize()
{
    //TODO
    return 0;
}

int32_t QCamera3MetadataChannel::request(camera3_stream_buffer_t *buffer)
{
    //TODO: If not already started, start streaming
    start();
    return 0;
}

int32_t QCamera3MetadataChannel::registerBuffers(uint32_t num_buffers, buffer_handle_t **buffers)
{
    //TODO
    return -EINVAL;
}

void QCamera3MetadataChannel::streamCbRoutine(mm_camera_super_buf_t *super_frame,
                            QCamera3Stream *stream)
{
    //TODO
    return;
}

QCamera3PicChannel::QCamera3PicChannel(uint32_t cam_handle,
                    mm_camera_ops_t *cam_ops,
                    channel_cb_routine cb_routine,
                    camera3_stream_t *stream) :
                        QCamera3Channel(cam_handle, cam_ops, cb_routine)
{
    //TODO
}

QCamera3PicChannel::~QCamera3PicChannel()
{
}

int32_t QCamera3PicChannel::request(camera3_stream_buffer_t *buffer)
{
    //TODO
    return 0;
}

int32_t QCamera3PicChannel::registerBuffers(uint32_t num_buffers,
                        buffer_handle_t **buffers)
{
    //TODO
    return 0;
}

void QCamera3PicChannel::streamCbRoutine(mm_camera_super_buf_t *super_frame,
                            QCamera3Stream *stream)
{
    //TODO
    return;
}

int QCamera3PicChannel::kMaxBuffers = 1;
}; // namespace qcamera
