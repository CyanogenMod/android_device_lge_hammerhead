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

#ifndef __QCAMERA2HARDWAREINTERFACE_H__
#define __QCAMERA2HARDWAREINTERFACE_H__

#include <utils/List.h>
#include <hardware/camera3.h>
#include <camera/CameraMetadata.h>

extern "C" {
#include <mm_camera_interface.h>
#include <mm_jpeg_interface.h>
}

using namespace android;

namespace qcamera {

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

class QCamera3MetadataChannel;

class QCamera3HardwareInterface {
public:
    /* static variable and functions accessed by camera service */
    static camera3_device_ops_t mCameraOps;
    static int initialize(const struct camera3_device *, const camera3_callback_ops_t *callback_ops);
    static int configure_streams(const struct camera3_device *, camera3_stream_configuration_t *stream_list);
    static int register_stream_buffers(const struct camera3_device *, const camera3_stream_buffer_set_t *buffer_set);
    static const camera_metadata_t* construct_default_request_settings(const struct camera3_device *, int type);
    static int process_capture_request(const struct camera3_device *, camera3_capture_request_t *request);
    static void get_metadata_vendor_tag_ops(const struct camera3_device *, vendor_tag_query_ops_t* ops);
    static void dump(const struct camera3_device *, int fd);

public:
    QCamera3HardwareInterface(int cameraId);
    virtual ~QCamera3HardwareInterface();
    int openCamera(struct hw_device_t **hw_device);
    int getMetadata(int type);
    camera_metadata_t* translateMetadata(int type);

    static int getCamInfo(int cameraId, struct camera_info *info);
    static int initCapabilities(int cameraId);
    static int initStaticMetadata(int cameraId);

    static void channelCbRoutine(mm_camera_buf_def_t *frame,
                camera3_stream_buffer_t *buffer, void *userdata);

    void sendCaptureResult(const struct camera3_callback_ops *, const camera3_capture_result_t *result);
    void notify(const struct camera3_callback_ops *, const camera3_notify_msg_t *msg);

    int initialize(const camera3_callback_ops_t *callback_ops);
    int configureStreams(camera3_stream_configuration_t *stream_list);
    int registerStreamBuffers(const camera3_stream_buffer_set_t *buffer_set);
private:

    int openCamera();
    int closeCamera();

public:

private:
    camera3_device_t   mCameraDevice;
    uint8_t            mCameraId;
    mm_camera_vtbl_t  *mCameraHandle;
    bool               mCameraOpened;
    camera_metadata_t *mDefaultMetadata[CAMERA3_TEMPLATE_COUNT];

    const camera3_callback_ops_t *mCallbackOps;

    camera3_stream_t *mInputStream;
    QCamera3MetadataChannel *mMetadataChannel;
};

}; // namespace qcamera

#endif /* __QCAMERA2HARDWAREINTERFACE_H__ */
