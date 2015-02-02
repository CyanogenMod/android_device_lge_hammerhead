/*
 * Copyright (c) 2015 The CyanogenMod Project
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

package org.cyanogenmod.doze.hammerhead;

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.os.SystemClock;
import android.util.Log;

public class TiltSensor extends HammerheadSensor {

    private static final boolean DEBUG = false;
    private static final String TAG = "TiltSensor";

    private static final int MIN_PULSE_INTERVAL_MS = 2500;

    private long mEntryTimestamp;
    private AccelSensor mAccelSensor;

    public TiltSensor(Context context) {
        super(context, Sensor.TYPE_TILT_DETECTOR);
    }

    @Override
    public void enable() {
        if (DEBUG) Log.d(TAG, "Enabling");
        super.enable();
        mEntryTimestamp = SystemClock.elapsedRealtime();
    }

    @Override
    public void disable() {
        if (DEBUG) Log.d(TAG, "Disabling");
        super.disable();
    }

    @Override
    protected void onSensorEvent(SensorEvent event) {
        if (DEBUG) Log.d(TAG, "Got sensor event: " + event.values[0]);

        long delta = SystemClock.elapsedRealtime() - mEntryTimestamp;
        if (delta < MIN_PULSE_INTERVAL_MS) {
            return;
        } else {
            mEntryTimestamp = SystemClock.elapsedRealtime();
        }

        if (event.values[0] == 1) {
            mAccelSensor = new AccelSensor(mContext);
            mAccelSensor.enable();
        }
    }
}
