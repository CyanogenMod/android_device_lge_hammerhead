# Copyright 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# LGE blob(s) necessary for Hammerhead hardware
PRODUCT_COPY_FILES := \
    vendor/lge/hammerhead/proprietary/Bluetooth_cal.acdb:system/etc/Bluetooth_cal.acdb:lge \
    vendor/lge/hammerhead/proprietary/DxHDCP.cfg:system/etc/DxHDCP.cfg:lge \
    vendor/lge/hammerhead/proprietary/General_cal.acdb:system/etc/General_cal.acdb:lge \
    vendor/lge/hammerhead/proprietary/Global_cal.acdb:system/etc/Global_cal.acdb:lge \
    vendor/lge/hammerhead/proprietary/Handset_cal.acdb:system/etc/Handset_cal.acdb:lge \
    vendor/lge/hammerhead/proprietary/Hdmi_cal.acdb:system/etc/Hdmi_cal.acdb:lge \
    vendor/lge/hammerhead/proprietary/Headset_cal.acdb:system/etc/Headset_cal.acdb:lge \
    vendor/lge/hammerhead/proprietary/Speaker_cal.acdb:system/etc/Speaker_cal.acdb:lge \
    vendor/lge/hammerhead/proprietary/vss_init:system/vendor/bin/vss_init:lge \
    vendor/lge/hammerhead/proprietary/bu24205_LGIT_VER_2_CAL.bin:system/vendor/firmware/bu24205_LGIT_VER_2_CAL.bin:lge \
    vendor/lge/hammerhead/proprietary/bu24205_LGIT_VER_2_DATA1.bin:system/vendor/firmware/bu24205_LGIT_VER_2_DATA1.bin:lge \
    vendor/lge/hammerhead/proprietary/bu24205_LGIT_VER_2_DATA2.bin:system/vendor/firmware/bu24205_LGIT_VER_2_DATA2.bin:lge \
    vendor/lge/hammerhead/proprietary/bu24205_LGIT_VER_2_DATA3.bin:system/vendor/firmware/bu24205_LGIT_VER_2_DATA3.bin:lge \
    vendor/lge/hammerhead/proprietary/bu24205_LGIT_VER_3_CAL.bin:system/vendor/firmware/bu24205_LGIT_VER_3_CAL.bin:lge \
    vendor/lge/hammerhead/proprietary/bu24205_LGIT_VER_3_DATA1.bin:system/vendor/firmware/bu24205_LGIT_VER_3_DATA1.bin:lge \
    vendor/lge/hammerhead/proprietary/bu24205_LGIT_VER_3_DATA2.bin:system/vendor/firmware/bu24205_LGIT_VER_3_DATA2.bin:lge \
    vendor/lge/hammerhead/proprietary/bu24205_LGIT_VER_3_DATA3.bin:system/vendor/firmware/bu24205_LGIT_VER_3_DATA3.bin:lge \
    vendor/lge/hammerhead/proprietary/dxhdcp2.b00:system/vendor/firmware/discretix/dxhdcp2.b00:lge \
    vendor/lge/hammerhead/proprietary/dxhdcp2.b01:system/vendor/firmware/discretix/dxhdcp2.b01:lge \
    vendor/lge/hammerhead/proprietary/dxhdcp2.b02:system/vendor/firmware/discretix/dxhdcp2.b02:lge \
    vendor/lge/hammerhead/proprietary/dxhdcp2.b03:system/vendor/firmware/discretix/dxhdcp2.b03:lge \
    vendor/lge/hammerhead/proprietary/dxhdcp2.mdt:system/vendor/firmware/discretix/dxhdcp2.mdt:lge \
    vendor/lge/hammerhead/proprietary/keymaster.b00:system/vendor/firmware/keymaster/keymaster.b00:lge \
    vendor/lge/hammerhead/proprietary/keymaster.b01:system/vendor/firmware/keymaster/keymaster.b01:lge \
    vendor/lge/hammerhead/proprietary/keymaster.b02:system/vendor/firmware/keymaster/keymaster.b02:lge \
    vendor/lge/hammerhead/proprietary/keymaster.b03:system/vendor/firmware/keymaster/keymaster.b03:lge \
    vendor/lge/hammerhead/proprietary/keymaster.mdt:system/vendor/firmware/keymaster/keymaster.mdt:lge \
    vendor/lge/hammerhead/proprietary/libAKM8963.so:system/vendor/lib/libAKM8963.so:lge \
    vendor/lge/hammerhead/proprietary/libDxHdcp.so:system/vendor/lib/libDxHdcp.so:lge \
    vendor/lge/hammerhead/proprietary/libvss_common_core.so:system/vendor/lib/libvss_common_core.so:lge \
    vendor/lge/hammerhead/proprietary/libvss_common_idl.so:system/vendor/lib/libvss_common_idl.so:lge \
    vendor/lge/hammerhead/proprietary/libvss_common_iface.so:system/vendor/lib/libvss_common_iface.so:lge \

