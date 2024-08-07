/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <vitasdk.h>

#include "../client/client.h"
#include "../sys/sys_local.h"

static int hires_x, hires_y;
/*
===============
IN_Frame
===============
*/
uint32_t oldkeys, oldanalogs;
void Key_Event(int key, int value, int time){
	Com_QueueEvent(time, SE_KEY, key, value, 0, NULL);
}

void Sys_SetKeys(uint32_t keys, int time){
	if((keys & SCE_CTRL_START) != (oldkeys & SCE_CTRL_START))
		Key_Event(K_ESCAPE, (keys & SCE_CTRL_START) == SCE_CTRL_START, time);
	if((keys & SCE_CTRL_SELECT) != (oldkeys & SCE_CTRL_SELECT))
		Key_Event(K_ENTER, (keys & SCE_CTRL_SELECT) == SCE_CTRL_SELECT, time);
	if((keys & SCE_CTRL_UP) != (oldkeys & SCE_CTRL_UP))
		Key_Event(K_UPARROW, (keys & SCE_CTRL_UP) == SCE_CTRL_UP, time);
	if((keys & SCE_CTRL_DOWN) != (oldkeys & SCE_CTRL_DOWN))
		Key_Event(K_DOWNARROW, (keys & SCE_CTRL_DOWN) == SCE_CTRL_DOWN, time);
	if((keys & SCE_CTRL_LEFT) != (oldkeys & SCE_CTRL_LEFT))
		Key_Event(K_LEFTARROW, (keys & SCE_CTRL_LEFT) == SCE_CTRL_LEFT, time);
	if((keys & SCE_CTRL_RIGHT) != (oldkeys & SCE_CTRL_RIGHT))
		Key_Event(K_RIGHTARROW, (keys & SCE_CTRL_RIGHT) == SCE_CTRL_RIGHT, time);
	if((keys & SCE_CTRL_TRIANGLE) != (oldkeys & SCE_CTRL_TRIANGLE))
		Key_Event(K_AUX4, (keys & SCE_CTRL_TRIANGLE) == SCE_CTRL_TRIANGLE, time);
	if((keys & SCE_CTRL_SQUARE) != (oldkeys & SCE_CTRL_SQUARE))
		Key_Event(K_AUX3, (keys & SCE_CTRL_SQUARE) == SCE_CTRL_SQUARE, time);
	if((keys & SCE_CTRL_CIRCLE) != (oldkeys & SCE_CTRL_CIRCLE))
		Key_Event(K_AUX2, (keys & SCE_CTRL_CIRCLE) == SCE_CTRL_CIRCLE, time);
	if((keys & SCE_CTRL_CROSS) != (oldkeys & SCE_CTRL_CROSS))
		Key_Event(K_AUX1, (keys & SCE_CTRL_CROSS) == SCE_CTRL_CROSS, time);
	if((keys & SCE_CTRL_LTRIGGER) != (oldkeys & SCE_CTRL_LTRIGGER))
		Key_Event(K_AUX5, (keys & SCE_CTRL_LTRIGGER) == SCE_CTRL_LTRIGGER, time);
	if((keys & SCE_CTRL_RTRIGGER) != (oldkeys & SCE_CTRL_RTRIGGER))
		Key_Event(K_AUX6, (keys & SCE_CTRL_RTRIGGER) == SCE_CTRL_RTRIGGER, time);
}

void IN_RescaleAnalog(int *x, int *y, int dead) {

	float analogX = (float) *x;
	float analogY = (float) *y;
	float deadZone = (float) dead;
	float maximum = 32768.0f;
	float magnitude = sqrt(analogX * analogX + analogY * analogY);
	if (magnitude >= deadZone)
	{
		float scalingFactor = maximum / magnitude * (magnitude - deadZone) / (maximum - deadZone);
		*x = (int) (analogX * scalingFactor);
		*y = (int) (analogY * scalingFactor);
	} else {
		*x = 0;
		*y = 0;
	}
}

// Left analog virtual values
#define LANALOG_LEFT  0x01
#define LANALOG_RIGHT 0x02
#define LANALOG_UP    0x04
#define LANALOG_DOWN  0x08

int old_x = - 1, old_y;

extern char title_keyboard[256];
extern uint8_t is_ime_up;
static uint8_t is_ime_up_old = 0;

void simulateKeyPress(char *c) {
	if (c[0] == 0)
		return;
	
	int time = Sys_Milliseconds();
	
	//We first delete the current text
	for (int i = 0; i < 100; i++) {
		Com_QueueEvent(time, SE_CHAR, 0x08, 0, 0, NULL );
	}
	
	while (*c) {
		int utf32 = 0;

		if( ( *c & 0x80 ) == 0 )
			utf32 = *c++;
		else if( ( *c & 0xE0 ) == 0xC0 ) // 110x xxxx
		{
			utf32 |= ( *c++ & 0x1F ) << 6;
			utf32 |= ( *c++ & 0x3F );
		}
		else if( ( *c & 0xF0 ) == 0xE0 ) // 1110 xxxx
		{
			utf32 |= ( *c++ & 0x0F ) << 12;
			utf32 |= ( *c++ & 0x3F ) << 6;
			utf32 |= ( *c++ & 0x3F );
		}
		else if( ( *c & 0xF8 ) == 0xF0 ) // 1111 0xxx
		{
			utf32 |= ( *c++ & 0x07 ) << 18;
			utf32 |= ( *c++ & 0x3F ) << 12;
			utf32 |= ( *c++ & 0x3F ) << 6;
			utf32 |= ( *c++ & 0x3F );
		}
		else
		{
			Com_DPrintf( "Unrecognised UTF-8 lead byte: 0x%x\n", (unsigned int)*c );
			c++;
		}
		
		Com_QueueEvent(time, SE_CHAR, utf32, 0, 0, NULL );
	}
}


void IN_Frame( void )
{
	if (is_ime_up_old && !is_ime_up) {
		simulateKeyPress(title_keyboard);
	}
	is_ime_up_old = is_ime_up;

	SceCtrlData keys;
	sceCtrlPeekBufferPositive(0, &keys, 1);
	int time = Sys_Milliseconds();
	if(keys.buttons != oldkeys)
		Sys_SetKeys(keys.buttons, time);
	oldkeys = keys.buttons;
	
	// Emulating mouse with touch
	SceTouchData touch;
	sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);
	if (touch.reportNum > 0){
		if (old_x != -1) Com_QueueEvent(time, SE_MOUSE, (touch.report[0].x - old_x), (touch.report[0].y - old_y), 0, NULL);
		old_x = touch.report[0].x;
		old_y = touch.report[0].y;
	}else old_x = -1;
	
	// Emulating mouse with right analog
	int right_x = (keys.rx - 127) * 256;
	int right_y = (keys.ry - 127) * 256;
	IN_RescaleAnalog(&right_x, &right_y, 7680);
	hires_x += right_x;
	hires_y += right_y;
	if (hires_x != 0 || hires_y != 0) {
		// increase slowdown variable to slow down aiming
		Com_QueueEvent(time, SE_MOUSE, hires_x / (int)cl_analog_slowdown->value, hires_y / (int)cl_analog_slowdown->value, 0, NULL);
		hires_x %= (int)cl_analog_slowdown->value;
		hires_y %= (int)cl_analog_slowdown->value;
	}

	// Gyroscope aiming
	if (cl_gyroscope->value) {
		SceMotionState motionstate;
		sceMotionGetState(&motionstate);

		// not sure why YAW or the horizontal x axis is the controlled by angularVelocity.y
		// and the PITCH or the vertical y axis is controlled by angularVelocity.x but its what seems to work
		float x_gyro_cam = 10 * motionstate.angularVelocity.y * cl_gyro_h_sensitivity->value;
		float y_gyro_cam = 10 * motionstate.angularVelocity.x * cl_gyro_v_sensitivity->value;
		Com_QueueEvent(time, SE_MOUSE, -x_gyro_cam, -y_gyro_cam, 0, NULL);
	}

	// Emulating keys with left analog (TODO: Replace this dirty hack with a serious implementation)
	uint32_t virt_buttons = 0x00;
	if (keys.lx < 80) virt_buttons += LANALOG_LEFT;
	else if (keys.lx > 160) virt_buttons += LANALOG_RIGHT;
	if (keys.ly < 80) virt_buttons += LANALOG_UP;
	else if (keys.ly > 160) virt_buttons += LANALOG_DOWN;
	if (virt_buttons != oldanalogs){
		if((virt_buttons & LANALOG_LEFT) != (oldanalogs & LANALOG_LEFT))
			Key_Event(K_AUX7, (virt_buttons & LANALOG_LEFT) == LANALOG_LEFT, time);
		if((virt_buttons & LANALOG_RIGHT) != (oldanalogs & LANALOG_RIGHT))
			Key_Event(K_AUX8, (virt_buttons & LANALOG_RIGHT) == LANALOG_RIGHT, time);
		if((virt_buttons & LANALOG_UP) != (oldanalogs & LANALOG_UP))
			Key_Event(K_AUX9, (virt_buttons & LANALOG_UP) == LANALOG_UP, time);
		if((virt_buttons & LANALOG_DOWN) != (oldanalogs & LANALOG_DOWN))
			Key_Event(K_AUX10, (virt_buttons & LANALOG_DOWN) == LANALOG_DOWN, time);
	}
	oldanalogs = virt_buttons;
	
}

/*
===============
IN_Init
===============
*/
void IN_Init( void *windowData )
{
	sceMotionReset();
	sceMotionStartSampling();
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG_WIDE);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);
}

/*
===============
IN_Shutdown
===============
*/
void IN_Shutdown( void )
{
}

/*
===============
IN_Restart
===============
*/
void IN_Restart( void )
{
}
