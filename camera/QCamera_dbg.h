#ifndef __QCAMERA_DBG_H__
#define __QCAMERA_DBG_H__

#define LOG_DEBUG 0 

#if !(LOG_DEBUG)
#define LOG_NDEBUG 1

#undef ALOGI
#define ALOGI(...)      ALOGV(__VA_ARGS__)
#undef ALOGD
#define ALOGD(...)	ALOGV(__VA_ARGS__)
#endif

#endif	/* __QCAMERA_DBG_H__ */
