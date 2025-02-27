/*****************************************************************************
 * Copyright (c) 2014 Ted John
 * OpenRCT2, an open source clone of Roller Coaster Tycoon 2.
 * 
 * This file is part of OpenRCT2.
 * 
 * OpenRCT2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************/

#include <SDL_keycode.h>
#include "addresses.h"
#include "audio/audio.h"
#include "config.h"
#include "game.h"
#include "input.h"
#include "interface/keyboard_shortcut.h"
#include "interface/viewport.h"
#include "interface/widget.h"
#include "interface/window.h"
#include "localisation/localisation.h"
#include "platform/osinterface.h"
#include "ride/ride_data.h"
#include "scenario.h"
#include "tutorial.h"
#include "windows/tooltip.h"
#include "windows/dropdown.h"
#include "world/banner.h"
#include "world/map.h"
#include "world/sprite.h"
#include "world/scenery.h"

static int _dragX, _dragY;
static rct_windowclass _dragWindowClass;
static rct_windownumber _dragWindowNumber;
static int _dragWidgetIndex;
static int _originalWindowWidth, _originalWindowHeight;

typedef struct {
	uint32 x, y;
	uint32 state; //1 = LeftDown 2 = LeftUp 3 = RightDown 4 = RightUp
} rct_mouse_data;

rct_mouse_data* mouse_buffer = RCT2_ADDRESS(RCT2_ADDRESS_INPUT_QUEUE, rct_mouse_data);

static void game_get_next_input(int *x, int *y, int *state);
static void input_widget_over(int x, int y, rct_window *w, int widgetIndex);
static void input_widget_over_change_check(rct_windowclass windowClass, rct_windownumber windowNumber, int widgetIndex);
static void input_widget_over_flatbutton_invalidate();
void process_mouse_over(int x, int y);
void process_mouse_tool(int x, int y);
void invalidate_scroll();
static rct_mouse_data* get_mouse_input();
void map_element_right_click(int type, rct_map_element *mapElement, int x, int y);
int sub_6EDE88(int x, int y, rct_map_element **mapElement, int *outX, int *outY);
int get_next_key();
static void game_handle_input_mouse(int x, int y, int state);
void game_handle_edge_scroll();
void game_handle_key_scroll();
static void input_widget_left(int x, int y, rct_window *w, int widgetIndex);
void input_state_widget_pressed(int x, int y, int state, int widgetIndex, rct_window* w, rct_widget* widget);
void sub_6ED990(char cursor_id);
static void input_window_position_begin(rct_window *w, int widgetIndex, int x, int y);
static void input_window_position_continue(rct_window *w, int lastX, int lastY, int newX, int newY);
static void input_window_position_end(rct_window *w, int x, int y);
static void input_window_resize_begin(rct_window *w, int widgetIndex, int x, int y);
static void input_window_resize_continue(rct_window *w, int x, int y);
static void input_window_resize_end();
static void input_viewport_drag_begin(rct_window *w, int x, int y);
static void input_viewport_drag_continue();
static void input_viewport_drag_end();
static void input_scroll_begin();
static void input_scroll_continue(rct_window *w, int widgetIndex, int state, int x, int y);
static void input_scroll_end();
static void input_update_tooltip(rct_window *w, int widgetIndex, int x, int y);

#pragma region Mouse input

/**
 * 
 *  rct2: 0x006EA627
 */
void game_handle_input()
{
	rct_window *w;
	int x, y, state;

	if (RCT2_GLOBAL(0x009DEA64, uint16) & 2) {
		RCT2_GLOBAL(0x009DEA64, uint16) &= ~2;
		game_do_command(0, 1, 0, 0, GAME_COMMAND_LOAD_OR_QUIT, 2, 0); 
	}

	if (RCT2_GLOBAL(0x009ABDF2, uint8) != 0) {
		for (w = g_window_list; w < RCT2_GLOBAL(RCT2_ADDRESS_NEW_WINDOW_PTR, rct_window*); w++)
			RCT2_CALLPROC_X(w->event_handlers[WE_UNKNOWN_07], 0, 0, 0, 0, (int)w, 0, 0);

		sub_6EA73F();
		RCT2_CALLPROC_EBPSAFE(0x006E8346); // update_cursor_position

		for (;;) {
			game_get_next_input(&x, &y, &state);
			if (state == 0)
				break;

			game_handle_input_mouse(x, y, state & 0xFF);
		}

		if (RCT2_GLOBAL(RCT2_ADDRESS_INPUT_FLAGS, uint32) & INPUT_FLAG_5) {
			game_handle_input_mouse(x, y, state);
		} else if (x != 0x80000000) {
			x = clamp(0, x, RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_WIDTH, sint16) - 1);
			y = clamp(0, y, RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_HEIGHT, sint16) - 1);

			game_handle_input_mouse(x, y, state);
			process_mouse_over(x, y);
			process_mouse_tool(x, y);
		}
	}

	for (w = g_window_list; w < RCT2_GLOBAL(RCT2_ADDRESS_NEW_WINDOW_PTR, rct_window*); w++)
		RCT2_CALLPROC_X(w->event_handlers[WE_UNKNOWN_08], 0, 0, 0, 0,(int) w, 0, 0);
}

/**
 * 
 *  rct2: 0x006E83C7
 */
static void game_get_next_input(int *x, int *y, int *state)
{
	rct_mouse_data* eax = get_mouse_input();
	if (eax == NULL) {
		*x = gCursorState.x;
		*y = gCursorState.y;
		*state = 0;
		return;
	}

	*x = eax->x;
	*y = eax->y;
	*state = eax->state;

	//int eax, ebx, ecx, edx, esi, edi, ebp;
	//RCT2_CALLFUNC_X(0x006E83C7, &eax, &ebx, &ecx, &edx, &esi, &edi, &ebp);
	//*x = eax & 0xFFFF;
	//*y = ebx & 0xFFFF;
	//*state = ecx & 0xFF;
	//return;

	//int on_tutorial = RCT2_GLOBAL(RCT2_ADDRESS_ON_TUTORIAL, uint8);
	//if (RCT2_GLOBAL(RCT2_ADDRESS_INPUT_FLAGS, uint32) & INPUT_FLAG_5) {
	//	if (on_tutorial == 1) {

	//	} else {
	//		RCT2_CALLPROC_EBPSAFE(0x00407074);
	//	}
	//	if (on_tutorial == 2) {

	//	}

	//} else {

	//}
}

/**
 *
 *  rct2: 0x00407074
 */
static rct_mouse_data* get_mouse_input()
{
	uint32 read_index = RCT2_GLOBAL(RCT2_ADDRESS_MOUSE_READ_INDEX, uint32);

	// check if that location has been written to yet
	if (read_index == RCT2_GLOBAL(RCT2_ADDRESS_MOUSE_WRITE_INDEX, uint32))
		return NULL;

	RCT2_GLOBAL(RCT2_ADDRESS_MOUSE_READ_INDEX, uint32) = (read_index + 1) % 64;
	return &mouse_buffer[read_index];
}

/**
 * 
 *  rct2: 0x006E8655
 */
static void game_handle_input_mouse(int x, int y, int state)
{
	rct_window *w;
	rct_widget *widget;
	int widgetIndex;

	// Get window and widget under cursor position
	w = window_find_from_point(x, y);
	widgetIndex = w == NULL ? -1 : window_find_widget_from_point(w, x, y);
	widget = widgetIndex == -1 ? 0 : &w->widgets[widgetIndex];

	switch (RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8)) {
	case INPUT_STATE_RESET:
		window_tooltip_reset(x, y);
		// fall-through
	case INPUT_STATE_NORMAL:
		switch (state) {
		case 0:
			input_widget_over(x, y, w, widgetIndex);
			break;
		case 1:
			input_widget_left(x, y, w, widgetIndex);
			break;
		case 3:
			window_close_by_class(WC_TOOLTIP);
			
			if (w != NULL)
				w = window_bring_to_front(w);
			
			if (widgetIndex == -1)
				break;
			
			if (widget->type == WWT_VIEWPORT) {
				if (!(RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & (SCREEN_FLAGS_TRACK_MANAGER | SCREEN_FLAGS_TITLE_DEMO)))
					input_viewport_drag_begin(w, x, y);
			} else if (widget->type == WWT_SCROLL) {

			}
			break;
		}

		break;
	case INPUT_STATE_WIDGET_PRESSED:
		input_state_widget_pressed(x, y, state, widgetIndex, w, widget);
		break;
	case INPUT_STATE_POSITIONING_WINDOW:
		w = window_find_by_number(_dragWindowClass, _dragWindowNumber);
		if (w == NULL) {
			RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_RESET;
		} else {
			input_window_position_continue(w, _dragX, _dragY, x, y);
			if (state == 2)
				input_window_position_end(w, x, y);
		}
		break;
	case INPUT_STATE_VIEWPORT_RIGHT:
		if (state == 0) {
			input_viewport_drag_continue();
		} else if (state == 4) {
			input_viewport_drag_end();
			if (RCT2_GLOBAL(0x009DE540, sint16) < 500) {
				viewport_interaction_right_click(
					RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DRAG_LAST_X, sint16),
					RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DRAG_LAST_Y, sint16)
				);
			}
		}
		break;
	case INPUT_STATE_DROPDOWN_ACTIVE:
		input_state_widget_pressed(x, y, state, widgetIndex, w, widget);
		break;
	case INPUT_STATE_VIEWPORT_LEFT:
		//RCT2_CALLPROC_X(0x006E87B4, x, y, state, widgetIndex, (int)w, (int)widget, 0);
		w = window_find_by_number(
			RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DRAG_WINDOWCLASS, rct_windowclass),
			RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DRAG_WINDOWNUMBER, rct_windownumber)
		);
		if (!w){
			RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = 0;
			break;
		}

		if (state == 0){
			if (!w->viewport){
				RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = 0;
				break;
			}

			if (w->classification != RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DRAG_WINDOWCLASS, rct_windowclass) ||
				w->number != RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DRAG_WINDOWNUMBER, rct_windownumber) ||
				!(RCT2_GLOBAL(0x9DE518, uint32)&(1 << 3)))break;
			
			w = window_find_by_number(
				RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WINDOWCLASS, rct_windowclass),
				RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WINDOWNUMBER, rct_windownumber)
			);
			if (w == NULL)
				break;

			RCT2_CALLPROC_X(w->event_handlers[WE_TOOL_DRAG], x, y, 0, (int)RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WIDGETINDEX, uint16), (int)w, 0, 0);
		}
		else if (state == 2){
			
			RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = 0;
			if (RCT2_GLOBAL(0x9DE52E, rct_windownumber) != w->number)break;
			if ((RCT2_GLOBAL(0x9DE518, uint32)&(1 << 3))){
				w = window_find_by_number(
					RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WINDOWCLASS, rct_windowclass),
					RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WINDOWNUMBER, rct_windownumber)
				);
				if (w == NULL)
					break;
				RCT2_CALLPROC_X(w->event_handlers[WE_TOOL_UP], x, y, 0, (int)RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WIDGETINDEX, uint16), (int)w, 0, 0);
			}
			else{
				if ((RCT2_GLOBAL(0x9DE518, uint32) & (1 << 4)))
					break;

				viewport_interaction_left_click(x, y);
			}
		}
		break;
	case INPUT_STATE_SCROLL_LEFT:
		if (state == 0)
			input_scroll_continue(w, widgetIndex, state, x, y);
		else if (state == 2)
			input_scroll_end();
		break;
	case INPUT_STATE_RESIZING:
		w = window_find_by_number(_dragWindowClass, _dragWindowNumber);
		if (w == NULL) {
			RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_RESET;
		} else {
			if (state == 2)
				input_window_resize_end();

			if (state == 0 || state == 2)
				input_window_resize_continue(w, x, y);
		}
		break;
	case 9:
		RCT2_CALLPROC_X(0x006E8ACB, x, y, state, widgetIndex, (int)w, (int)widget, 0);
		break;
	}
}

#pragma region Window positioning / resizing

static void input_window_position_begin(rct_window *w, int widgetIndex, int x, int y)
{
	RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_POSITIONING_WINDOW;
	_dragX = x - w->x;
	_dragY = y - w->y;
	_dragWindowClass = w->classification;
	_dragWindowNumber = w->number;
	_dragWidgetIndex = widgetIndex;
}

static void input_window_position_continue(rct_window *w, int wdx, int wdy, int x, int y)
{
	window_move_and_snap(w, x - wdx, y - wdy, gGeneral_config.window_snap_proximity);
}

static void input_window_position_end(rct_window *w, int x, int y)
{
	RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_NORMAL;
	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_TIMEOUT, uint8) = 0;
	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WINDOW_CLASS, rct_windowclass) = _dragWindowClass;
	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WINDOW_NUMBER, rct_windownumber) = _dragWindowNumber;
	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WIDGET_INDEX, uint16) = _dragWidgetIndex;
	RCT2_CALLPROC_X(w->event_handlers[WE_UNKNOWN_18], 0, 0, x, y, (int)w, 0, 0);
}

static void input_window_resize_begin(rct_window *w, int widgetIndex, int x, int y)
{
	RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_RESIZING;
	_dragX = x;
	_dragY = y;
	_dragWindowClass = w->classification;
	_dragWindowNumber = w->number;
	_dragWidgetIndex = widgetIndex;
	_originalWindowWidth = w->width;
	_originalWindowHeight = w->height;
}

static void input_window_resize_continue(rct_window *w, int x, int y)
{
	int dx, dy, targetWidth, targetHeight;

	if (y < RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_HEIGHT, uint16) - 2) {
		dx = x - _dragX;
		dy = y - _dragY;
		targetWidth = _originalWindowWidth + dx;
		targetHeight = _originalWindowHeight + dy;

		window_resize(
			w,
			targetWidth - w->width,
			targetHeight - w->height
		);
	}
}

static void input_window_resize_end()
{
	RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_NORMAL;
	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_TIMEOUT, uint8) = 0;
	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WINDOW_CLASS, rct_windowclass) = _dragWindowClass;
	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WINDOW_NUMBER, rct_windownumber) = _dragWindowNumber;
	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WIDGET_INDEX, uint16) = _dragWidgetIndex;
}

#pragma endregion

#pragma region Viewport dragging

static void input_viewport_drag_begin(rct_window *w, int x, int y)
{
	w->flags &= ~WF_SCROLLING_TO_LOCATION;
	RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_VIEWPORT_RIGHT;
	_dragWindowClass = w->classification;
	_dragWindowNumber = w->number;
	RCT2_GLOBAL(0x009DE540, sint16) = 0;
	platform_get_cursor_position(&_dragX, &_dragY);
	platform_hide_cursor();

	// RCT2_GLOBAL(RCT2_ADDRESS_INPUT_FLAGS, uint32) |= INPUT_FLAG_5;
}

static void input_viewport_drag_continue()
{
	int dx, dy, newDragX, newDragY;
	rct_window *w;
	rct_viewport *viewport;

	platform_get_cursor_position(&newDragX, &newDragY);

	dx = newDragX - _dragX;
	dy = newDragY - _dragY;
	w = window_find_by_number(_dragWindowClass, _dragWindowNumber);

	viewport = w->viewport;
	RCT2_GLOBAL(0x009DE540, sint16) += RCT2_GLOBAL(0x009DE588, sint16);
	if (viewport == NULL) {
		platform_show_cursor();
		RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_RESET;
	} else if (dx != 0 || dy != 0) {
		if (!(w->flags & (1 << 2))) {
			RCT2_GLOBAL(0x009DE540, sint16) = 1000;
			dx <<= viewport->zoom + 1;
			dy <<= viewport->zoom + 1;
			w->saved_view_x += dx;
			w->saved_view_y += dy;
		}
	}

	platform_set_cursor_position(_dragX, _dragY);
}

static void input_viewport_drag_end()
{
	RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_RESET;
	platform_show_cursor();
}

#pragma endregion

#pragma region Scroll bars

static void input_scroll_begin(rct_window *w, int widgetIndex, int x, int y)
{
	rct_widget *widget;

	widget = &w->widgets[widgetIndex];

	RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_SCROLL_LEFT;
	RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WIDGETINDEX, uint16) = widgetIndex;
	RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WINDOWCLASS, rct_windowclass) = w->classification;
	RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WINDOWNUMBER, rct_windownumber) = w->number;
	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_CURSOR_X, uint16) = x;
	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_CURSOR_Y, uint16) = y;

	int eax, ebx, scroll_area, scroll_id;
	scroll_id = 0; // safety
	widget_scroll_get_part(w, widget, x, y, &eax, &ebx, &scroll_area, &scroll_id);

	RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_SCROLL_AREA, uint16) = scroll_area;
	RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_SCROLL_ID, uint32) = scroll_id * sizeof(rct_scroll);//We do this because scroll id is not all decompiled
	RCT2_CALLPROC_X(w->event_handlers[WE_UNKNOWN_15], RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_SCROLL_ID, uint32), ebx, scroll_area, scroll_id, (int)w, (int)widget, 0);
	if (scroll_area == SCROLL_PART_VIEW){
		RCT2_CALLPROC_X(w->event_handlers[WE_SCROLL_MOUSEDOWN], scroll_id / sizeof(rct_scroll), ebx, eax, ebx, (int)w, (int)widget, 0);
		return;
	}

	rct_widget* widg = &w->widgets[widgetIndex];
	rct_scroll* scroll = &w->scrolls[scroll_id];

	int widget_width = widg->right - widg->left - 1;
	if (scroll->flags & VSCROLLBAR_VISIBLE)
		widget_width -= 11;
	widget_width = max(scroll->h_right - widget_width, 0);

	int widget_height = widg->bottom - widg->top - 1;
	if (scroll->flags & HSCROLLBAR_VISIBLE)
		widget_height -= 11;
	widget_height = max(scroll->v_bottom - widget_height, 0);

	switch (scroll_area) {
	case SCROLL_PART_HSCROLLBAR_LEFT:
		scroll->h_left = max(scroll->h_left - 3, 0);
		break;
	case SCROLL_PART_HSCROLLBAR_RIGHT:
		scroll->h_left = min(scroll->h_left + 3, widget_width);
		break;
	case SCROLL_PART_HSCROLLBAR_LEFT_TROUGH: 
		scroll->h_left = max(scroll->h_left - widget_width , 0);
		break;
	case SCROLL_PART_HSCROLLBAR_RIGHT_TROUGH:
		scroll->h_left = min(scroll->h_left + widget_width, widget_width);
		break;
	case SCROLL_PART_VSCROLLBAR_TOP:
		scroll->v_top = max(scroll->v_top - 3, 0);
		break;
	case SCROLL_PART_VSCROLLBAR_BOTTOM:
		scroll->v_top = min(scroll->v_top + 3, widget_height);
		break;
	case SCROLL_PART_VSCROLLBAR_TOP_TROUGH:
		scroll->v_top = max(scroll->v_top - widget_height, 0);
		break;
	case SCROLL_PART_VSCROLLBAR_BOTTOM_TROUGH:
		scroll->v_top = min(scroll->v_top + widget_height, widget_height);
		break;
	default:
		break;
	}
	widget_scroll_update_thumbs(w, widgetIndex);
	window_invalidate_by_number(widgetIndex, w->classification);
}

static void input_scroll_continue(rct_window *w, int widgetIndex, int state, int x, int y)
{
	rct_widget *widget;

	widget = &w->widgets[widgetIndex];
	if (widgetIndex != RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WIDGETINDEX, uint32)){
		invalidate_scroll();
		return;
	}
	if (w->classification != RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WINDOWCLASS, uint8)){
		invalidate_scroll();
		return;
	}
	if (w->number != RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WINDOWNUMBER, uint16)){
		invalidate_scroll();
		return;
	}

	if (RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_SCROLL_AREA, uint16) == SCROLL_PART_HSCROLLBAR_THUMB){
		int temp_x = x;
		x -= RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_CURSOR_X, uint16);
		RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_CURSOR_X, uint16) = temp_x;
		RCT2_CALLPROC_X(0x006E98F2, x, temp_x, state, w->number, (int)w, (int)widget, x);
		return;
	}

	if (RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_SCROLL_AREA, uint16) == SCROLL_PART_VSCROLLBAR_THUMB){
		int temp_y = y;	
		y -= RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_CURSOR_Y, uint16);
		RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_CURSOR_Y, uint16) = temp_y;
		RCT2_CALLPROC_X(0x006E99A9, temp_y, y, state, w->number, (int)w, (int)widget, y);
		return;
	}
	int scroll_part, scroll_id;
	widget_scroll_get_part(w, widget, x, y, &x, &y, &scroll_part, &scroll_id);

	if (scroll_part != RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_SCROLL_AREA, uint16)){
		invalidate_scroll();
		return;
	}

	switch (scroll_part){
	case SCROLL_PART_VIEW:
		RCT2_CALLPROC_X(w->event_handlers[WE_TOOL_DRAG], w->number / 18, y, x, y, (int)w, (int)widget, (int)w->event_handlers);
		break;
	case SCROLL_PART_HSCROLLBAR_LEFT:
		RCT2_CALLPROC_X(0x006E9A60, x, y, scroll_part, w->number, (int)w, (int)widget, 0);
		break;
	case SCROLL_PART_HSCROLLBAR_RIGHT:
		RCT2_CALLPROC_X(0x006E9ABF, x, y, scroll_part, w->number, (int)w, (int)widget, 0);
		break;
	case SCROLL_PART_HSCROLLBAR_LEFT_TROUGH:
	case SCROLL_PART_HSCROLLBAR_RIGHT_TROUGH:
		return;
		break;
	case SCROLL_PART_HSCROLLBAR_THUMB:
	case SCROLL_PART_VSCROLLBAR_TOP:
		RCT2_CALLPROC_X(0x006E9C37, x, y, scroll_part, w->number, (int)w, (int)widget, 0);
		break;
	case SCROLL_PART_VSCROLLBAR_BOTTOM:
		RCT2_CALLPROC_X(0x006E9C96, x, y, scroll_part, w->number, (int)w, (int)widget, 0);
		break;
	case SCROLL_PART_VSCROLLBAR_TOP_TROUGH:
	case SCROLL_PART_VSCROLLBAR_BOTTOM_TROUGH:
		return;
		break;
	default:
		return;
	}
}

static void input_scroll_end()
{
	RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_RESET;
	invalidate_scroll();
}

#pragma endregion

#pragma region Widgets

/**
 * 
 *  rct2: 0x006E9253
 */
static void input_widget_over(int x, int y, rct_window *w, int widgetIndex)
{
	rct_windowclass windowClass = 255;
	rct_windownumber windowNumber = 0;
	rct_widget *widget = NULL;

	if (w != NULL) {
		windowClass = w->classification;
		windowNumber = w->number;
		widget = &w->widgets[widgetIndex];
	}

	input_widget_over_change_check(windowClass, windowNumber, widgetIndex);

	if (w != NULL && widgetIndex != -1 && widget->type == WWT_SCROLL) {
		int eax, ebx, ecx, edx;
		widget_scroll_get_part(w, widget, x, y, &eax, &ebx, &ecx, &edx);
		
		if (ecx < 0)
			input_update_tooltip(w, widgetIndex, x, y);
		else if (ecx == 0) {
			RCT2_CALLPROC_X(w->event_handlers[WE_SCROLL_MOUSEOVER], edx, 0, eax, ebx, (int)w, 0, 0);
			input_update_tooltip(w, widgetIndex, x, y);
		}
	} else {
		input_update_tooltip(w, widgetIndex, x, y);
	}

	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_TIMEOUT, uint16) = 0;
	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_CURSOR_X, sint16) = x;
	RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_CURSOR_Y, sint16) = y;
}

/**
 * 
 *  rct2: 0x006E9269
 */
static void input_widget_over_change_check(rct_windowclass windowClass, rct_windownumber windowNumber, int widgetIndex)
{
	// Prevents invalid widgets being clicked source of bug is elsewhere
	if (widgetIndex == -1)
		return;

	// Check if the widget that the cursor was over, has changed
	if (
		windowClass != RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_OVER_WINDOWCLASS, rct_windowclass) ||
		windowNumber != RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_OVER_WINDOWNUMBER, rct_windownumber) ||
		widgetIndex != RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_OVER_WIDGETINDEX, uint16)
	) {
		// Invalidate last widget cursor was on if widget is a flat button
		input_widget_over_flatbutton_invalidate();

		// Set new cursor over widget
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_OVER_WINDOWCLASS, rct_windowclass) = windowClass;
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_OVER_WINDOWNUMBER, rct_windownumber) = windowNumber;
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_OVER_WIDGETINDEX, uint16) = widgetIndex;

		// Invalidate new widget cursor is on if widget is a flat button
		if (windowClass != 255)
			input_widget_over_flatbutton_invalidate();
	}
}

/**
 * Used to invalidate flat button widgets when the mouse leaves and enters them. This should be generalised so that all widgets
 * can use this in the future.
 */
static void input_widget_over_flatbutton_invalidate()
{
	rct_window *w = window_find_by_number(
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_OVER_WINDOWCLASS, rct_windowclass),
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_OVER_WINDOWNUMBER, rct_windownumber)
	);
	if (w == NULL)
		return;

	window_event_helper(w, 0, WE_INVALIDATE);
	if (w->widgets[RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_OVER_WIDGETINDEX, rct_windownumber)].type == WWT_FLATBTN) {
		widget_invalidate_by_number(
			RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_OVER_WINDOWCLASS, rct_windowclass),
			RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_OVER_WINDOWNUMBER, rct_windownumber),
			RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_OVER_WIDGETINDEX, rct_windownumber)
		);
	}
}

/**
 * 
 *  rct2: 0x006E95F9
 */
static void input_widget_left(int x, int y, rct_window *w, int widgetIndex)
{
	rct_windowclass windowClass = 255;
	rct_windownumber windowNumber = 0;
	rct_widget *widget;

	if (w != NULL) {
		windowClass = w->classification;
		windowNumber = w->number;
	}

	window_close_by_class(WC_ERROR);
	window_close_by_class(WC_TOOLTIP);

	// Window might have changed position in the list, therefore find it again
	w = window_find_by_number(windowClass, windowNumber);
	if (w == NULL)
		return;

	w = window_bring_to_front(w);
	if (widgetIndex == -1)
		return;

	widget = &w->widgets[widgetIndex];

	switch (widget->type) {
	case WWT_FRAME:
	case WWT_RESIZE:
		if (window_can_resize(w) && (x >= w->x + w->width - 19 && y >= w->y + w->height - 19))
			input_window_resize_begin(w, widgetIndex, x, y);
		break;
	case WWT_VIEWPORT:
		RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_VIEWPORT_LEFT;
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DRAG_LAST_X, uint16) = x;
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DRAG_LAST_Y, uint16) = y;
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DRAG_WINDOWCLASS, rct_windowclass) = windowClass;
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DRAG_WINDOWNUMBER, rct_windownumber) = windowNumber;
		if (RCT2_GLOBAL(RCT2_ADDRESS_INPUT_FLAGS, uint32) & INPUT_FLAG_TOOL_ACTIVE) {
			w = window_find_by_number(
				RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WINDOWCLASS, rct_windowclass),
				RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WINDOWNUMBER, rct_windownumber)
			);
			if (w != NULL) {
				RCT2_CALLPROC_X(w->event_handlers[WE_TOOL_DOWN], x, y, 0, RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WIDGETINDEX, uint16), (int)w, 0, 0);
				RCT2_GLOBAL(RCT2_ADDRESS_INPUT_FLAGS, uint32) |= INPUT_FLAG_4;
			}
		}
		break;
	case WWT_CAPTION:
		input_window_position_begin(w, widgetIndex, x, y);
		break;
	case WWT_SCROLL:
		input_scroll_begin(w, widgetIndex, x, y);
		break;
	default:
		if (widget_is_enabled(w, widgetIndex) && !widget_is_disabled(w, widgetIndex)) {
			sound_play_panned(SOUND_CLICK_1, w->x + (widget->left + widget->right) / 2, 0, 0, 0);

			// Set new cursor down widget
			RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WINDOWCLASS, rct_windowclass) = windowClass;
			RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WINDOWNUMBER, rct_windownumber) = windowNumber;
			RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WIDGETINDEX, uint16) = widgetIndex;
			RCT2_GLOBAL(RCT2_ADDRESS_INPUT_FLAGS, uint32) |= INPUT_FLAG_WIDGET_PRESSED;
			RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = INPUT_STATE_WIDGET_PRESSED;
			RCT2_GLOBAL(0x009DE528, uint16) = 1;

			widget_invalidate_by_number(windowClass, windowNumber, widgetIndex);
			window_event_helper(w, widgetIndex, WE_MOUSE_DOWN);
		}
		break;
	}
}

#pragma endregion

/**
*
*  rct2: 0x006ED833
*/
void process_mouse_over(int x, int y)
{
	rct_window* window;
	rct_window* subWindow;

	int widgetId;
	int cursorId;
	int eax, ebx, ecx, edx, esi, edi, ebp;

	cursorId = CURSOR_ARROW;
	RCT2_GLOBAL(0x9A9808, sint16) = -1;
	window = window_find_from_point(x, y);

	if (window != NULL) {
		widgetId = window_find_widget_from_point(window, x, y);
		RCT2_GLOBAL(0x1420046, sint16) = (widgetId & 0xFFFF);
		if (widgetId != -1) {
			switch (window->widgets[widgetId].type){
			case WWT_VIEWPORT:
				if ((RCT2_GLOBAL(0x9DE518, int) & 0x8) == 0) {
					if (viewport_interaction_left_over(x, y)) {
						sub_6ED990(CURSOR_HAND_POINT);
						return;
					}
					break;
				}
				cursorId = RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_TOOL, uint8);
				subWindow = window_find_by_number(
					RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WINDOWCLASS, rct_windowclass),
					RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WINDOWNUMBER, rct_windownumber)
				);
				ebp = (int)subWindow;
				if (subWindow == NULL)
				{
					break;
				}
				ebx = ebx & 0xFFFFFF00;
				edi = cursorId;
				esi = (int) subWindow;
				RCT2_CALLFUNC_X(subWindow->event_handlers[WE_UNKNOWN_0E], &eax, &ebx, &ecx, &edx, &esi, &edi, &ebp);
				cursorId = edi;
				if ((ebx & 0xFF) != 0)
				{
					sub_6ED990(cursorId);
					return;
				}
				break;
			case WWT_FRAME:
			case WWT_RESIZE:
				if (!(window->flags & 0x100))
				{
					break;
				}
				if (window->min_width == window->max_width && window->min_height == window->max_height)
				{
					break;
				}
				if (x < window->x + window->width - 0x13)
				{
					break;
				}
				if (y < window->y + window->height - 0x13)
				{
					break;
				}
				cursorId = CURSOR_DIAGONAL_ARROWS;
				break;
			case WWT_SCROLL:
				RCT2_GLOBAL(0x9DE558, uint16) = x;
				RCT2_GLOBAL(0x9DE55A, uint16) = y;
				int output_x, output_y, output_scroll_area, scroll_id;
				widget_scroll_get_part(window, window->widgets, x, y, &output_x, &output_y, &output_scroll_area, &scroll_id);
				cursorId = scroll_id;
				if (output_scroll_area != SCROLL_PART_VIEW)
				{
					cursorId = CURSOR_ARROW;
					break;
				}
				//Fall through to default
			default:
				ecx = x;
				edx = y;
				eax = widgetId;
				ebx = 0xFFFFFFFF;
				edi = (int)&window->widgets[widgetId];

				RCT2_CALLFUNC_X(window->event_handlers[WE_UNKNOWN_17], &eax, &ebx, &ecx, &edx, &esi, &edi, &ebp);
				if (ebx == 0xFFFFFFFF)
				{
					cursorId = CURSOR_ARROW;
					break;
				}
				cursorId = ebx;
				break;
			}
		}
	}

	viewport_interaction_right_over(x, y);
	sub_6ED990(cursorId);
}

/**
 *
 *  rct2: 0x006ED801
 */
void process_mouse_tool(int x, int y)
{
	if (RCT2_GLOBAL(0x9DE518, uint32) & (1 << 3))
	{
		rct_window* w = window_find_by_number(
			RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WINDOWCLASS, uint8),
			RCT2_GLOBAL(RCT2_ADDRESS_TOOL_WINDOWNUMBER, uint16)
		);

		if (!w)
			tool_cancel();
		else
			RCT2_CALLPROC_X(w->event_handlers[WE_TOOL_UPDATE], x, y, 0, RCT2_GLOBAL(0x9DE546, uint16), (int)w, 0, 0);

	}
}

/**
 * 
 *  rct2: 0x006E8DA7
 */
void input_state_widget_pressed( int x, int y, int state, int widgetIndex, rct_window* w, rct_widget* widget ){
	//RCT2_CALLPROC_X(0x006E8DA7, x, y, state, widgetIndex, (int)w, (int)widget, 0);
	//return;
	RCT2_GLOBAL(0x1420054, uint16) = x;
	RCT2_GLOBAL(0x1420056, uint16) = y;

	rct_windowclass cursor_w_class;
	rct_windownumber cursor_w_number;
	cursor_w_class = RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WINDOWCLASS, rct_windowclass);
	cursor_w_number = RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WINDOWNUMBER, rct_windownumber);
	int cursor_widgetIndex = RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WIDGETINDEX, uint32);

	rct_window* cursor_w = window_find_by_number(cursor_w_class, cursor_w_number);
	if (cursor_w == NULL) {
		RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = 0;
		return;
	}

	switch (state){
	case 0:
		if (!w || cursor_w_class != w->classification || cursor_w_number != w->number || widgetIndex != cursor_widgetIndex)
			break;

		if (w->disabled_widgets & (1ULL << widgetIndex))
			break;

		if (RCT2_GLOBAL(0x9DE528, uint16) != 0) RCT2_GLOBAL(0x9DE528, uint16)++;

		if (w->var_020 & (1ULL << widgetIndex) &&
			RCT2_GLOBAL(0x9DE528, uint16) >= 0x10 &&
			(!(RCT2_GLOBAL(0x9DE528, uint16) & 0x3))){
			window_event_helper(w, widgetIndex, WE_MOUSE_DOWN);
		}

		if (RCT2_GLOBAL(0x9DE518, uint32) & 1) return;

		RCT2_GLOBAL(0x9DE518, uint32) |= 1;
		widget_invalidate_by_number(cursor_w_class, cursor_w_number, widgetIndex);
		return;
	case 3:
	case 2:
		if (RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) == 5){
			if (w) {
				int dropdown_index = 0;

				if (w->classification == WC_DROPDOWN){
					dropdown_index = dropdown_index_from_point(x, y, w);
					if (dropdown_index == -1)goto dropdown_cleanup;

					// _dropdown_unknown?? highlighted?
					if (dropdown_index < 32 && RCT2_GLOBAL(0x009DED34, sint32) & (1 << dropdown_index))goto dropdown_cleanup;

					// gDropdownItemsFormat[dropdown_index] will not work until all windows that use dropdown decompiled
					if (RCT2_ADDRESS(0x9DEBA4, uint16)[dropdown_index] == 0)goto dropdown_cleanup;
				}
				else{
					if (cursor_w_class != w->classification || cursor_w_number != w->number || widgetIndex != cursor_widgetIndex)
						goto dropdown_cleanup;
					dropdown_index = -1;
					if (RCT2_GLOBAL(0x9DE518, uint32) & 2){
						if (!(RCT2_GLOBAL(0x9DE518, uint32) & 4)){
							RCT2_GLOBAL(0x9DE518, uint32) |= (1 << 2);
							return;
						}
					}
				}

				window_close_by_class(WC_DROPDOWN);
				cursor_w = window_find_by_number(cursor_w_class, cursor_w_number);
				if (RCT2_GLOBAL(0x9DE518, uint32) & 1){
					RCT2_GLOBAL(0x9DE518, uint32) &= 0xFFFE;
					widget_invalidate_by_number(cursor_w_class, cursor_w_number, cursor_widgetIndex);
				}

				RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = 1;
				RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_TIMEOUT, uint16) = 0;
				RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WIDGET_INDEX, uint16) = cursor_widgetIndex;
				RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WINDOW_CLASS, rct_windowclass) = cursor_w_class;
				RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WINDOW_NUMBER, rct_windownumber) = cursor_w_number;
				RCT2_CALLPROC_X(cursor_w->event_handlers[WE_DROPDOWN], dropdown_index, 0, 0, cursor_widgetIndex, (int)cursor_w, 0, 0);
			}
		dropdown_cleanup:
			window_close_by_class(WC_DROPDOWN);
		}
		if (state == 3) return;

		RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) = 1;
		RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_TIMEOUT, uint16) = 0;
		RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WIDGET_INDEX, uint16) = cursor_widgetIndex;

		if (!w)
			break;

		int mid_point_x = (widget->left + widget->right) / 2 + w->x;
		sound_play_panned(5, mid_point_x, 0, 0, 0);
		if (cursor_w_class != w->classification || cursor_w_number != w->number || widgetIndex != cursor_widgetIndex)
			break;
		
		if (w->disabled_widgets & (1ULL << widgetIndex))
			break;

		widget_invalidate_by_number(cursor_w_class, cursor_w_number, widgetIndex);
		window_event_helper(w, widgetIndex, WE_MOUSE_UP);
	default:
		return;
	}

	RCT2_GLOBAL(0x9DE528, uint16) = 0;
	if (RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) != 5){
		// Hold down widget and drag outside of area??
		if (RCT2_GLOBAL(0x9DE518, uint32) & 1){
			RCT2_GLOBAL(0x9DE518, uint32) &= 0xFFFE;
			widget_invalidate_by_number(cursor_w_class, cursor_w_number, cursor_widgetIndex);
		}
		return;
	}

	if (!w) return;

	if (w->classification == WC_DROPDOWN){
		int dropdown_index = dropdown_index_from_point(x, y, w);

		if (dropdown_index == -1) return;

		// _dropdown_unknown?? highlighted?
		if (dropdown_index < 32 && RCT2_GLOBAL(0x009DED34, sint32) & (1 << dropdown_index))return;

		// gDropdownItemsFormat[dropdown_index] will not work until all windows that use dropdown decompiled
		if (RCT2_ADDRESS(0x9DEBA4, uint16)[dropdown_index] == 0)return;

		// _dropdown_highlighted_index
		RCT2_GLOBAL(0x009DEBA2, sint16) = dropdown_index;
		window_invalidate_by_class(WC_DROPDOWN);
	}
}

static void input_update_tooltip(rct_window *w, int widgetIndex, int x, int y)
{
	if (RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WINDOW_CLASS, rct_windowclass) == 255) {
		if (RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_NOT_SHOWN_TICKS, uint16) < 500 ||
			(RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_CURSOR_X, sint16) == x &&
			RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_CURSOR_Y, sint16) == y)
		) {
			RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_TIMEOUT, uint16) = RCT2_GLOBAL(0x009DE588, uint16);

			int bp = 2000;
			if (RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_NOT_SHOWN_TICKS, uint16) <= 1000)
				bp = 0;
			if (bp > RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_TIMEOUT, uint16))
				return;

			window_tooltip_open(w, widgetIndex, x, y);
			// RCT2_CALLPROC_X(0x006EA10D, x, y, 0, widgetIndex, w, widget, 0); // window_tooltip_open();
		}
	} else {
		if (RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WINDOW_CLASS, rct_windowclass) != w->classification ||
			RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WINDOW_NUMBER, rct_windownumber) != w->number ||
			RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_WIDGET_INDEX, uint16) != widgetIndex
		) {
			window_tooltip_close();
		}
		RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_TIMEOUT, uint16) += RCT2_GLOBAL(0x009DE588, uint16);
		if (RCT2_GLOBAL(RCT2_ADDRESS_TOOLTIP_TIMEOUT, uint16) < 8000)
			return;
		window_close_by_class(WC_TOOLTIP);
	}
}



#pragma endregion

#pragma region Keyboard input

/**
 * 
 *  rct2: 0x006E3B43
 */
void game_handle_keyboard_input()
{
	rct_window *w;
	int key;

	// Handle mouse scrolling
	if (RCT2_GLOBAL(RCT2_ADDRESS_ON_TUTORIAL, uint8) == 0)
		if (RCT2_GLOBAL(RCT2_ADDRESS_CONFIG_EDGE_SCROLLING, uint8) != 0)
			if (RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) == 1)
				if (!(RCT2_GLOBAL(RCT2_ADDRESS_PLACE_OBJECT_MODIFIER, uint8) & 3))
					game_handle_edge_scroll();

	// Handle modifier keys and key scrolling
	RCT2_GLOBAL(RCT2_ADDRESS_PLACE_OBJECT_MODIFIER, uint8) = 0;
	if (RCT2_GLOBAL(0x009E2B64, uint32) != 1) {
		if (gKeysState[SDL_SCANCODE_LSHIFT] || gKeysState[SDL_SCANCODE_RSHIFT])
			RCT2_GLOBAL(RCT2_ADDRESS_PLACE_OBJECT_MODIFIER, uint8) |= 1;
		if (gKeysState[SDL_SCANCODE_LCTRL] || gKeysState[SDL_SCANCODE_RCTRL])
			RCT2_GLOBAL(RCT2_ADDRESS_PLACE_OBJECT_MODIFIER, uint8) |= 2;
		if (RCT2_GLOBAL(RCT2_ADDRESS_ON_TUTORIAL, uint8) == 0)
			game_handle_key_scroll();
	}


	// Handle key input
	while ((key = get_next_key()) != 0) {
		if (key == 255)
			continue;

		key |= RCT2_GLOBAL(RCT2_ADDRESS_PLACE_OBJECT_MODIFIER, uint8) << 8;

		w = window_find_by_class(WC_CHANGE_KEYBOARD_SHORTCUT);
		if (w != NULL)
			keyboard_shortcut_set(key);
		else if (RCT2_GLOBAL(RCT2_ADDRESS_ON_TUTORIAL, uint8) == 1)
			tutorial_stop();
		else{
			w = window_find_by_class(WC_TEXTINPUT);
			if (w != NULL){
				((void(*)(int, rct_window*))w->event_handlers[WE_TEXT_INPUT])(key,w);
			}
			else{
				keyboard_shortcut_handle(key);
			}
		}
			
	}

	if (RCT2_GLOBAL(RCT2_ADDRESS_ON_TUTORIAL, uint8) == 0)
		return;

	// Tutorial and the modifier key
	if (RCT2_GLOBAL(RCT2_ADDRESS_ON_TUTORIAL, uint8) == 1) {
		int eax, ebx, ecx, edx, esi, edi, ebp;
		RCT2_CALLFUNC_X(0x0066EEB4, &eax, &ebx, &ecx, &edx, &esi, &edi, &ebp);
		eax &= 0xFF;
		RCT2_GLOBAL(RCT2_ADDRESS_PLACE_OBJECT_MODIFIER, uint8) = eax;
		if (RCT2_GLOBAL(RCT2_ADDRESS_PLACE_OBJECT_MODIFIER, uint8) & 4) {
			window_tooltip_close();
			if ((w = window_get_main()) != NULL) {
				RCT2_CALLPROC_X(0x006EA2AA, 0, 0, 0, 0, (int)w, RCT2_GLOBAL(0x009DEA72, uint16), 0);
				RCT2_GLOBAL(0x009DEA72, uint16)++;
			}
		}
	} else {
		if (!(RCT2_GLOBAL(RCT2_ADDRESS_PLACE_OBJECT_MODIFIER, uint8) & 4)) {
			window_tooltip_close();
			if ((w = window_get_main()) != NULL) {
				RCT2_CALLPROC_X(0x006EA2AA, 0, 0, 0, 0, (int)w, RCT2_GLOBAL(0x009DEA72, uint16), 0);
				RCT2_GLOBAL(0x009DEA72, uint16)++;
			}
		}

		// Write tutorial input
		RCT2_CALLPROC_X(0x0066EEE1, RCT2_GLOBAL(RCT2_ADDRESS_PLACE_OBJECT_MODIFIER, uint8), 0, 0, 0, 0, 0, 0);
	}
}

/**
 * 
 *  rct2: 0x00406CD2
 */
int get_next_key()
{
	int i;
	for (i = 0; i < 221; i++) {
		if (gKeysPressed[i]) {
			gKeysPressed[i] = 0;
			return i;
		}
	}

	return 0;
}

#pragma endregion

/**
*
*  rct2: 0x006ED990
*/
void sub_6ED990(char cursor_id){
	if (RCT2_GLOBAL(RCT2_ADDRESS_INPUT_STATE, uint8) == INPUT_STATE_RESIZING)
	{
		cursor_id = CURSOR_DIAGONAL_ARROWS;	//resize icon
	}

	if (cursor_id == RCT2_GLOBAL(RCT2_ADDRESS_CURENT_CURSOR, uint8))
	{
		return;
	}
	RCT2_GLOBAL(RCT2_ADDRESS_CURENT_CURSOR, uint8) = cursor_id;
	RCT2_GLOBAL(0x14241BC, uint32) = 2;
	osinterface_set_cursor(cursor_id);
	RCT2_GLOBAL(0x14241BC, uint32) = 0;
}



/**
 * 
 *  rct2: 0x006E876D
 */
void invalidate_scroll()
{
	rct_window* wind = window_find_by_number(
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WINDOWCLASS, uint8),
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WINDOWNUMBER, uint16)
	);
	if (wind == NULL) return;

	int scroll_id = RCT2_GLOBAL(RCT2_ADDRESS_CURRENT_SCROLL_ID, uint32);
	//Reset to basic scroll
	wind->scrolls[scroll_id / sizeof(rct_scroll)].flags &= 0xFF11;

	window_invalidate_by_number(
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WINDOWCLASS, uint8),
		RCT2_GLOBAL(RCT2_ADDRESS_CURSOR_DOWN_WINDOWNUMBER, uint16)
	);
}

/**
* rct2: 0x00406C96
*/
void store_mouse_input(int state)
{
	uint32 write_index = RCT2_GLOBAL(RCT2_ADDRESS_MOUSE_WRITE_INDEX, uint32);
	uint32 next_write_index = (write_index + 1) % 64;

	// check if the queue is full
	if (next_write_index == RCT2_GLOBAL(RCT2_ADDRESS_MOUSE_READ_INDEX, uint32))
		return;

	rct_mouse_data* item = &mouse_buffer[write_index];
	item->x = RCT2_GLOBAL(0x01424318, uint32);
	item->y = RCT2_GLOBAL(0x0142431C, uint32);
	item->state = state;

	RCT2_GLOBAL(RCT2_ADDRESS_MOUSE_WRITE_INDEX, uint32) = next_write_index;
}

void game_handle_edge_scroll()
{
	rct_window *mainWindow;
	int scrollX, scrollY;

	mainWindow = window_get_main();
	if (mainWindow == NULL)
		return;
	if ((mainWindow->flags & WF_2) || (RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & 9))
		return;
	if (mainWindow->viewport == NULL)
		return;

	scrollX = 0;
	scrollY = 0;

	// Scroll left / right
	if (gCursorState.x == 0)
		scrollX = -1;
	else if (gCursorState.x == RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_WIDTH, uint16) - 1)
		scrollX = 1;

	// Scroll up / down
	if (gCursorState.y == 0)
		scrollY = -1;
	else if (gCursorState.y == RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_HEIGHT, uint16) - 1)
		scrollY = 1;

	// Scroll viewport
	if (scrollX != 0) {
		mainWindow->saved_view_x += scrollX * (12 << mainWindow->viewport->zoom);
		RCT2_GLOBAL(RCT2_ADDRESS_INPUT_FLAGS, uint32) |= INPUT_FLAG_VIEWPORT_SCROLLING;
	}
	if (scrollY != 0) {
		mainWindow->saved_view_y += scrollY * (12 << mainWindow->viewport->zoom);
		RCT2_GLOBAL(RCT2_ADDRESS_INPUT_FLAGS, uint32) |= INPUT_FLAG_VIEWPORT_SCROLLING;
	}
}

void game_handle_key_scroll()
{
	rct_window *mainWindow;
	int scrollX, scrollY;

	mainWindow = window_get_main();
	if (mainWindow == NULL)
		return;
	if ((mainWindow->flags & WF_2) || (RCT2_GLOBAL(RCT2_ADDRESS_SCREEN_FLAGS, uint8) & 9))
		return;
	if (mainWindow->viewport == NULL)
		return;

	rct_window *textWindow;

	textWindow = window_find_by_class(WC_TEXTINPUT);
	if (textWindow) return;

	scrollX = 0;
	scrollY = 0;

	// Scroll left / right
	if (gKeysState[SDL_SCANCODE_LEFT])
		scrollX = -1;
	else if (gKeysState[SDL_SCANCODE_RIGHT])
		scrollX = 1;

	// Scroll up / down
	if (gKeysState[SDL_SCANCODE_UP])
		scrollY = -1;
	else if (gKeysState[SDL_SCANCODE_DOWN])
		scrollY = 1;

	// Scroll viewport
	if (scrollX != 0) {
		mainWindow->saved_view_x += scrollX * (12 << mainWindow->viewport->zoom);
		RCT2_GLOBAL(RCT2_ADDRESS_INPUT_FLAGS, uint32) |= INPUT_FLAG_VIEWPORT_SCROLLING;
	}
	if (scrollY != 0) {
		mainWindow->saved_view_y += scrollY * (12 << mainWindow->viewport->zoom);
		RCT2_GLOBAL(RCT2_ADDRESS_INPUT_FLAGS, uint32) |= INPUT_FLAG_VIEWPORT_SCROLLING;
	}
}