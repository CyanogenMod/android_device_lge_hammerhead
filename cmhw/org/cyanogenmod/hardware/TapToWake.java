/*
 * Copyright (C) 2014 The CyanogenMod Project
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

package org.cyanogenmod.hardware;

import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;

import android.util.Log;

import org.cyanogenmod.hardware.util.FileUtils;

public class TapToWake {

    private static String CONTROL_PATH = "/sys/android_touch/doubletap2wake";

    public static boolean isSupported() {
        return new File(CONTROL_PATH).exists();
    }

    public static boolean isEnabled() {
        return (Integer.parseInt(FileUtils.readOneLine(CONTROL_PATH)) == 1);
    }

    public static boolean setEnabled(boolean state) {
        Runtime rt = Runtime.getRuntime();
        Process process;
        try {
            process = rt.exec("su");
            DataOutputStream os = new DataOutputStream(process.getOutputStream());
            os.writeBytes("echo " + (state ? "1" : "0") + " > " + CONTROL_PATH + "\n");
            os.writeBytes("exit\n");
            os.flush();
            process.waitFor();
        } catch (IOException e) {
            Log.e("TapToWake", "Couldn't write doubletap2wake");
            return false;
        } catch (InterruptedException e) {
        }

        return true;
    }
}
