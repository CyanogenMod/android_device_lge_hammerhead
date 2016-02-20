#!/bin/bash
#
# Copyright (C) 2013-2015 The CyanogenMod Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#


set -e

VENDOR=lge
DEVICE=hammerhead

if [ $# -eq 0 ]; then
  SRC=adb
else
  if [ $# -eq 1 ]; then
    SRC=$1
  else
    echo "$0: bad number of arguments"
    echo ""
    echo "usage: $0 [PATH_TO_EXPANDED_ROM]"
    echo ""
    echo "If PATH_TO_EXPANDED_ROM is not specified, blobs will be extracted from"
    echo "the device using adb pull."
    exit 1
  fi
fi


BASE=../../../vendor/$VENDOR/$DEVICE/proprietary
rm -rf $BASE/*


for FILE in `cat proprietary-blobs.txt | grep -v ^# | grep -v ^$ | sed -e 's#^/system/##g' | sed -e "s#^-/system/##g"`; do
    DIR=`dirname $FILE`
    if [ ! -d $BASE/$DIR ]; then
        mkdir -p $BASE/$DIR
    fi

    if [ "$SRC" = "adb" ]; then
        if ! adb shell ls /system/$FILE | grep "No such file" &>/dev/null; then
            adb pull /system/$FILE $BASE/$FILE
        else
            echo "ERROR: Pull file /system/$FILE from a device running CyanogenMod"
        fi
    else
        if [ -e $SRC/$FILE ]; then
            cp $SRC/$FILE $BASE/$FILE
        else
            echo "ERROR: Pull file /system/$FILE from a device running CyanogenMod"
        fi
    fi
done

./setup-makefiles.sh
