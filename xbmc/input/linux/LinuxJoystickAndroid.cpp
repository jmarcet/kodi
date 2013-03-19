/*
 *      Copyright (C) 2013 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "LinuxJoystickAndroid.h"

#include "android/activity/JNIThreading.h"
#include "android/activity/AndroidExtra.h"

CLinuxJoystickAndroid::~CLinuxJoystickAndroid()
{
}

/* static */
void CLinuxJoystickAndroid::Initialize(JoystickArray &joysticks)
{
  JNIEnv* env = xbmc_jnienv();
  
  jclass cInputDevice = env->FindClass("android/view/InputDevice");
  jmethodID midGetDeviceIds = env->GetStaticMethodID(cInputDevice, "getDeviceIds", "()[I");
  jmethodID midGetDevice = env->GetStaticMethodID(cInputDevice, "getDevice", "(I)Landroid/view/InputDevice;");
  jmethodID midGetName = env->GetMethodID(cInputDevice, "getName", "()Ljava/lang/String;");
  jmethodID midGetSources = env->GetMethodID(cInputDevice, "getSources", "()I");
  jmethodID midGetMotionRanges = env->GetMethodID(cInputDevice, "getMotionRanges", "()Ljava/util/List;");

  jintArray retarr = (jintArray) env->CallStaticObjectMethod(cInputDevice, midGetDeviceIds);
  jsize len = env->GetArrayLength(retarr);
  jint *ids = env->GetIntArrayElements(retarr, 0);
  for (int i=0; i<len; i++)
  {
    jobject oInputDevice = env->CallStaticObjectMethod(cInputDevice, midGetDevice, ids[i]);
    int sources = env->CallIntMethod(oInputDevice, midGetSources);
    env->DeleteLocalRef(oInputDevice);
    
    if (sources & AINPUT_SOURCE_GAMEPAD || sources & AINPUT_SOURCE_JOYSTICK)
    {
      CLinuxJoystickAndroid* joy = new CLinuxJoystickAndroid();
      joy->m_state.id = ids[i];
      
      jstring sDeviceName = (jstring)env->CallObjectMethod(oInputDevice, midGetName);
      const char *nativeString = env->GetStringUTFChars(sDeviceName, 0);
      joy->m_state.name = std::string(nativeString);
      env->ReleaseStringUTFChars(sDeviceName, nativeString);
      env->DeleteLocalRef(sDeviceName);

      joysticks.push_back(boost::shared_ptr<IJoystick>(joy));
    }
  }
  
  env->DeleteLocalRef(retarr);
  env->DeleteLocalRef(cInputDevice);
}

void CLinuxJoystickAndroid::DeInitialize(JoystickArray &joysticks)
{
  for (int i = 0; i < (int)joysticks.size(); i++)
  {
    if (boost::dynamic_pointer_cast<CLinuxJoystickAndroid>(joysticks[i]))
    { 
      joysticks.erase(joysticks.begin() + i--);
    }
  }
}

void CLinuxJoystickAndroid::Update()
{
}
