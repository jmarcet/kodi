#pragma once
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

#include <stdint.h>
#include <android/input.h>

#include <string>
#include <map>

struct CAndroidJoystickDevice
{
  std::string name;
  int32_t sources;
  int32_t numaxis;
  std::map<int32_t, float> axisvalues;
};

class CAndroidJoystick
{
public:
  CAndroidJoystick();
  ~CAndroidJoystick();
  bool onJoystickMoveEvent(AInputEvent* event);
  bool onJoystickButtonEvent(AInputEvent* event);
  
protected:
  std::map<int32_t, CAndroidJoystickDevice*>::iterator addJoystick(int32_t deviceid);
  
private:
  std::map<int32_t, CAndroidJoystickDevice*> m_joysticks;
};