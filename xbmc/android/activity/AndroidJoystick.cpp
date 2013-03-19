/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "AndroidJoystick.h"
#include "XBMCApp.h"
#include "AndroidExtra.h"

#include "JNIThreading.h"

#include <dlfcn.h>

extern float AMotionEvent_getAxisValue(const AInputEvent* motion_event, int32_t axis, size_t pointer_index);
static typeof(AMotionEvent_getAxisValue) *p_AMotionEvent_getAxisValue;
#define AMotionEvent_getAxisValue (*p_AMotionEvent_getAxisValue)

CAndroidJoystick::CAndroidJoystick()
{
  p_AMotionEvent_getAxisValue = (typeof(AMotionEvent_getAxisValue)*) dlsym(RTLD_DEFAULT, "AMotionEvent_getAxisValue");
}

CAndroidJoystick::~CAndroidJoystick()
{
  std::map<int32_t, CAndroidJoystickDevice*>::iterator it;
  while (!m_joysticks.empty())
  {
    it = m_joysticks.begin();
    CAndroidJoystickDevice* device = it->second;
    delete device;
    m_joysticks.erase(it);
  }
}

bool CAndroidJoystick::onJoystickMoveEvent(AInputEvent* event)
{
  CXBMCApp::android_printf("%s", __PRETTY_FUNCTION__);
  if (event == NULL)
    return false;

  int32_t deviceid = AInputEvent_getDeviceId(event);
  std::map<int32_t, CAndroidJoystickDevice*>::iterator it = m_joysticks.find(deviceid);
  if (it == m_joysticks.end())
    it = addJoystick(deviceid);
  if (it == m_joysticks.end())
    return false;
    
  CXBMCApp::android_printf("CAndroidJoystick: move event (devname: %s; device:%d, src:%d)", it->second->name.c_str(), deviceid, AInputEvent_getSource(event));

  return true;
}

bool CAndroidJoystick::onJoystickButtonEvent(AInputEvent* event)
{
  CXBMCApp::android_printf("%s", __PRETTY_FUNCTION__);
  if (event == NULL)
    return false;

  int32_t deviceid = AInputEvent_getDeviceId(event);
  std::map<int32_t, CAndroidJoystickDevice*>::iterator it = m_joysticks.find(deviceid);
  if (it == m_joysticks.end())
    it = addJoystick(deviceid);
  if (it == m_joysticks.end())
    return false;

  int32_t keycode = AKeyEvent_getKeyCode(event);
  int32_t flags = AKeyEvent_getFlags(event);
  int32_t state = AKeyEvent_getMetaState(event);
  int32_t repeatCount = AKeyEvent_getRepeatCount(event);

  CXBMCApp::android_printf("CAndroidJoystick: button event (code: %d; repeat: %d; flags: 0x%0X; state: %d; devname: %s; device:%d, src:%d)", 
    keycode, repeatCount, flags, AKeyEvent_getAction(event), it->second->name.c_str(), deviceid, AInputEvent_getSource(event));

  switch (AKeyEvent_getAction(event))
  {
    case AKEY_EVENT_ACTION_DOWN:
      CXBMCApp::android_printf("CXBMCApp: key down (code: %d; repeat: %d; flags: 0x%0X; alt: %s; shift: %s; sym: %s)",
                      keycode, repeatCount, flags,
                      (state & AMETA_ALT_ON) ? "yes" : "no",
                      (state & AMETA_SHIFT_ON) ? "yes" : "no",
                      (state & AMETA_SYM_ON) ? "yes" : "no");
      return true;

    case AKEY_EVENT_ACTION_UP:
      CXBMCApp::android_printf("CXBMCApp: key up (code: %d; repeat: %d; flags: 0x%0X; alt: %s; shift: %s; sym: %s)",
                      keycode, repeatCount, flags,
                      (state & AMETA_ALT_ON) ? "yes" : "no",
                      (state & AMETA_SHIFT_ON) ? "yes" : "no",
                     (state & AMETA_SYM_ON) ? "yes" : "no");
      return true;

    case AKEY_EVENT_ACTION_MULTIPLE:
      CXBMCApp::android_printf("CXBMCApp: key multiple (code: %d; repeat: %d; flags: 0x%0X; alt: %s; shift: %s; sym: %s)",
                      keycode, repeatCount, flags,
                      (state & AMETA_ALT_ON) ? "yes" : "no",
                      (state & AMETA_SHIFT_ON) ? "yes" : "no",
                      (state & AMETA_SYM_ON) ? "yes" : "no");
      break;

    default:
      CXBMCApp::android_printf("CXBMCApp: unknown key (code: %d; repeat: %d; flags: 0x%0X; alt: %s; shift: %s; sym: %s)",
                      keycode, repeatCount, flags,
                      (state & AMETA_ALT_ON) ? "yes" : "no",
                      (state & AMETA_SHIFT_ON) ? "yes" : "no",
                      (state & AMETA_SYM_ON) ? "yes" : "no");
      break;
  }

  return false;
}

std::map<int32_t, CAndroidJoystickDevice*>::iterator CAndroidJoystick::addJoystick(int32_t deviceid)
{
  JNIEnv* env = xbmc_jnienv();
  
  jclass cInputDevice = env->FindClass("android/view/InputDevice");
  jmethodID midGetDevice = env->GetStaticMethodID(cInputDevice, "getDevice", "(I)Landroid/view/InputDevice;");
  jmethodID midGetName = env->GetMethodID(cInputDevice, "getName", "()Ljava/lang/String;");
  jmethodID midGetSources = env->GetMethodID(cInputDevice, "getSources", "()I");
  jmethodID midGetMotionRanges = env->GetMethodID(cInputDevice, "getMotionRanges", "()Ljava/util/List;");
  jobject oInputDevice = env->CallStaticObjectMethod(cInputDevice, midGetDevice, deviceid);

  jstring sDeviceName = (jstring)env->CallObjectMethod(oInputDevice, midGetName);

  if (sDeviceName == NULL)
    return m_joysticks.end();
  
  CAndroidJoystickDevice* device = new CAndroidJoystickDevice;

  const char *nativeString = env->GetStringUTFChars(sDeviceName, 0);
  device->name = std::string(nativeString);
  env->ReleaseStringUTFChars(sDeviceName, nativeString);
  env->DeleteLocalRef(sDeviceName);

  device->sources = env->CallIntMethod(oInputDevice, midGetSources);

  jclass cList = env->FindClass("java/util/List");
  jmethodID midListSize = env->GetMethodID(cList, "size", "()I");
  jmethodID midListGet = env->GetMethodID(cList, "get", "(I)Ljava/lang/Object;");
  
  jobject oListMotionRanges = env->CallObjectMethod(oInputDevice, midGetMotionRanges);
  device->numaxis = env->CallIntMethod(oListMotionRanges, midListSize);

  jclass cMotionRange = env->FindClass("android/view/InputDevice$MotionRange");
  jmethodID midGetAxis = env->GetMethodID(cMotionRange, "getAxis", "()I");
  jmethodID midGetSource = env->GetMethodID(cMotionRange, "getSource", "()I");
  
  for (int i=0; i<device->numaxis; ++i)
  {
    jobject oMotionRange = env->CallObjectMethod(oListMotionRanges, midListGet, i);
    int axisId = env->CallIntMethod(oMotionRange, midGetAxis);
    int src = env->CallIntMethod(oMotionRange, midGetSource);
    env->DeleteLocalRef(oMotionRange);
    
    if (src & AINPUT_SOURCE_GAMEPAD || src & AINPUT_SOURCE_JOYSTICK)
    {
      device->axisvalues.insert(std::pair<int32_t, float>(axisId, 0.0));
      CXBMCApp::android_printf(">> CAndroidJoystick: added axis: %d; src: %d (devname: %s; device:%d)", axisId, src, device->name.c_str(), deviceid);
    }
  }
  
  std::pair<std::map<int32_t, CAndroidJoystickDevice*>::iterator,bool> ret;
  ret = m_joysticks.insert(std::pair<int32_t, CAndroidJoystickDevice*>(deviceid, device));

  env->DeleteLocalRef(oListMotionRanges);
  env->DeleteLocalRef(oInputDevice);
  env->DeleteLocalRef(cMotionRange);
  env->DeleteLocalRef(cList);
  env->DeleteLocalRef(cInputDevice);
  
  return ret.first;
}