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

#include "input/JoystickManager.h"
#include "input/linux/LinuxJoystickAndroid.h"

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
}

bool CAndroidJoystick::onJoystickMoveEvent(AInputEvent* event)
{
  CXBMCApp::android_printf("%s", __PRETTY_FUNCTION__);
  if (event == NULL)
    return false;

  int32_t deviceid = AInputEvent_getDeviceId(event);
  CLinuxJoystickAndroid* joy = CLinuxJoystickAndroid::getJoystick(deviceid);
  if (joy == NULL)
  {
    CJoystickManager::Get().Reinitialize();
    joy = CLinuxJoystickAndroid::getJoystick(deviceid);
  }
  if (joy == NULL)
    return false;
    
  CXBMCApp::android_printf("CAndroidJoystick: move event (devname: %s; device:%d, src:%d)", joy->m_state.name.c_str(), deviceid, AInputEvent_getSource(event));

  return true;
}

bool CAndroidJoystick::onJoystickButtonEvent(AInputEvent* event)
{
  CXBMCApp::android_printf("%s", __PRETTY_FUNCTION__);
  if (event == NULL)
    return false;

  int32_t deviceid = AInputEvent_getDeviceId(event);
  CLinuxJoystickAndroid* joy = CLinuxJoystickAndroid::getJoystick(deviceid);
  if (joy == NULL)
  {
    CJoystickManager::Get().Reinitialize();
    joy = CLinuxJoystickAndroid::getJoystick(deviceid);
  }
  if (joy == NULL)
    return false;

  int32_t keycode = AKeyEvent_getKeyCode(event);
  int32_t flags = AKeyEvent_getFlags(event);
  int32_t state = AKeyEvent_getMetaState(event);
  int32_t repeatCount = AKeyEvent_getRepeatCount(event);

  CXBMCApp::android_printf("CAndroidJoystick: button event (code: %d; repeat: %d; flags: 0x%0X; state: %d; devname: %s; device:%d, src:%d)", 
    keycode, repeatCount, flags, AKeyEvent_getAction(event), joy->m_state.name.c_str(), deviceid, AInputEvent_getSource(event));

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
