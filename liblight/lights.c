/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#define LOG_TAG "lights"
#define DEBUG 0

#include <cutils/log.h>

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/ioctl.h>
#include <sys/types.h>

#include <hardware/lights.h>

/******************************************************************************/

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

char const *const LCD_FILE           = "/sys/class/leds/lcd-backlight/brightness";
char const *const RED_LED_FILE       = "/sys/class/leds/red/brightness";
char const *const GREEN_LED_FILE     = "/sys/class/leds/green/brightness";
char const *const BLUE_LED_FILE      = "/sys/class/leds/blue/brightness";
char const *const RED_TIMEOUT_FILE   = "/sys/class/leds/red/on_off_ms";
char const *const GREEN_TIMEOUT_FILE = "/sys/class/leds/green/on_off_ms";
char const *const BLUE_TIMEOUT_FILE  = "/sys/class/leds/blue/on_off_ms";
char const *const RGB_LOCKED_FILE    = "/sys/class/leds/red/rgb_start";

struct led_config {
    unsigned int colorRGB;
    int onMS, offMS;
};

static struct led_config g_leds[3]; // For battery, notifications, and attention.
static int g_cur_led = -1;          // Presently showing LED of the above.

/**
 * device methods
 */

void init_globals(void)
{
    // init the mutex
    pthread_mutex_init(&g_lock, NULL);
}

static int write_int(char const* path, int value)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[32] = {0,};
        int bytes = snprintf(buffer, sizeof(buffer), "%d\n", value);
        int amt = write(fd, buffer, bytes);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            ALOGE("write_int failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}

static int write_on_off(char const* path, int on, int off)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[32] = {0,};
        int bytes = snprintf(buffer, sizeof(buffer), "%d %d\n", on, off);
        int amt = write(fd, buffer, bytes);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            ALOGE("write_int failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}

static int rgb_to_brightness(struct light_state_t const* state)
{
    int color = state->color & 0x00ffffff;

    return ((77 * ((color >> 16) & 0x00ff))
            + (150 * ((color >> 8) & 0x00ff)) + (29 * (color & 0x00ff))) >> 8;
}

static int set_light_backlight(struct light_device_t* dev,
        struct light_state_t const* state)
{
    int err = 0;
    int brightness = rgb_to_brightness(state);

    pthread_mutex_lock(&g_lock);
    err = write_int(LCD_FILE, brightness);
    pthread_mutex_unlock(&g_lock);

    return err;
}

static int write_leds_locked(struct led_config *led) {
    static const struct led_config led_off = {0, 0, 0};

    if (led == NULL) {
        led = (struct led_config *) &led_off;
    }

    int red = (led->colorRGB >> 16) & 0xFF;
    int green = (led->colorRGB >> 8) & 0xFF;
    int blue = led->colorRGB & 0xFF;

    write_int(RGB_LOCKED_FILE, 0);

    write_int(RED_LED_FILE, red);
    write_int(GREEN_LED_FILE, green);
    write_int(BLUE_LED_FILE, blue);

    write_on_off(RED_TIMEOUT_FILE, led->onMS, led->offMS);
    write_on_off(GREEN_TIMEOUT_FILE, led->onMS, led->offMS);
    write_on_off(BLUE_TIMEOUT_FILE, led->onMS, led->offMS);

    write_int(RGB_LOCKED_FILE, 1);

    return 0;
}

static int set_light_locked(struct light_state_t const* state, int type)
{
    int red, green, blue;
    struct led_config *led;
    int err = 0;

    if (type < 0 || (unsigned int)type >= sizeof(g_leds)/sizeof(g_leds[0]))
        return -EINVAL;

    /* type is one of:
     *   0. battery
     *   1. notifications
     *   2. attention
     * which are multiplexed onto the same physical LED in the above order. */
    led = &g_leds[type];

    switch (state->flashMode) {
    case LIGHT_FLASH_TIMED:
    case LIGHT_FLASH_HARDWARE:
        led->onMS = state->flashOnMS;
        led->offMS = state->flashOffMS;
        break;
    case LIGHT_FLASH_NONE:
    default:
        led->onMS = 0;
        led->offMS = 0;
        break;
    }

#if DEBUG
    ALOGD("set_light_locked: mode %d, color=%08X, onMS=%d, offMS=%d, type=%d\n",
            state->flashMode, state->color, led->onMS, led->offMS, type);
#endif

    led->colorRGB = state->color & 0x00ffffff;

    if (led->colorRGB > 0) {
        /* This LED is lit. */
        if (type >= g_cur_led) {
            /* And it has the highest priority, so show it. */
            err = write_leds_locked(led);
            g_cur_led = type;
        }
    } else {
        /* This LED is not (any longer) lit. */
        if (type == g_cur_led) {
            /* But it is currently showing, switch to a lower-priority LED. */
            int i;

            for (i = type-1; i >= 0; i--) {
                if (g_leds[i].colorRGB > 0) {
                    /* Found a lower-priority LED to switch to. */
                    err = write_leds_locked(&g_leds[i]);
                    goto switched;
                }
            }

            /* No LEDs are lit, turn off. */
            err = write_leds_locked(NULL);
switched:
            g_cur_led = i;
        }
    }

    return err;
}

static int set_light_battery(struct light_device_t* dev,
        struct light_state_t const* state)
{
    pthread_mutex_lock(&g_lock);
    set_light_locked(state, 0);
    pthread_mutex_unlock(&g_lock);

    return 0;
}

static int set_light_notifications(struct light_device_t* dev,
        struct light_state_t const* state)
{
    pthread_mutex_lock(&g_lock);
    set_light_locked(state, 1);
    pthread_mutex_unlock(&g_lock);

    return 0;
}

static int set_light_attention(struct light_device_t* dev,
        struct light_state_t const* state)
{
    struct light_state_t fixed;

    pthread_mutex_lock(&g_lock);

    memcpy(&fixed, state, sizeof(fixed));
    /* The framework does odd things with the attention lights, fix them up to
     * do something sensible here. */
    switch (fixed.flashMode) {
    case LIGHT_FLASH_NONE:
        /* LightsService.Light::stopFlashing calls with non-zero color. */
        fixed.color = 0;
        break;
    case LIGHT_FLASH_HARDWARE:
        /* PowerManagerService::setAttentionLight calls with onMS=3, offMS=0, which
         * just makes for a slightly-dimmer LED. */
        if (fixed.flashOnMS > 0 && fixed.flashOffMS == 0)
            fixed.flashMode = LIGHT_FLASH_NONE;
        break;
    }

    set_light_locked(&fixed, 2);
    pthread_mutex_unlock(&g_lock);

    return 0;
}


/** Close the lights device */
static int close_lights(struct light_device_t *dev)
{
    if (dev)
        free(dev);

    return 0;
}

/******************************************************************************/

/**
 * module methods
 */

/** Open a new instance of a lights device using name */
static int open_lights(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    int (*set_light)(struct light_device_t* dev,
            struct light_state_t const* state);

    if (!strcmp(LIGHT_ID_BACKLIGHT, name))
        set_light = set_light_backlight;
    else if (!strcmp(LIGHT_ID_NOTIFICATIONS, name))
        set_light = set_light_notifications;
    else if (!strcmp(LIGHT_ID_ATTENTION, name))
        set_light = set_light_attention;
    else if (!strcmp(LIGHT_ID_BATTERY, name))
        set_light = set_light_battery;
    else
        return -EINVAL;

    pthread_once(&g_init, init_globals);

    struct light_device_t *dev = malloc(sizeof(struct light_device_t));
    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = (int (*)(struct hw_device_t*))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t*)dev;

    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

/*
 * The lights Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "lights Module",
    .author = "Google, Inc.",
    .methods = &lights_module_methods,
};
