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

#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "../common.h"
#include "../drawing/drawing.h"
#include "../peep/peep.h"
#include "../ride/ride.h"
#include "../ride/vehicle.h"
#include "../world/park.h"

struct rct_window;
union rct_window_event;

typedef void wndproc(struct rct_window*, union rct_window_event*);

typedef uint8 rct_windowclass;
typedef uint16 rct_windownumber;

typedef struct {
	rct_windowclass classification;
	rct_windownumber number;
} window_identifier;

typedef struct {
	window_identifier window;
	int widget_index;
} widget_identifier;

/**
 * Widget structure
 * size: 0x10
 */
typedef struct {
	uint8 type;						// 0x00
	uint8 colour;					// 0x01
	sint16 left;					// 0x02
	sint16 right;					// 0x04
	sint16 top;						// 0x06
	sint16 bottom;					// 0x08
	uint32 image;					// 0x0A
	uint16 tooltip;					// 0x0E
} rct_widget;

/**
 * Viewport structure
 * size: 0x14
 */
typedef struct {
	sint16 width;					// 0x00
	sint16 height;					// 0x02
	sint16 x;						// 0x04
	sint16 y;						// 0x06
	sint16 view_x;					// 0x08
	sint16 view_y;					// 0x0A
	sint16 view_width;				// 0x0C
	sint16 view_height;				// 0x0E
	uint8 zoom;						// 0x10
	uint8 var_11;
	uint16 flags;					// 0x12
} rct_viewport;

/**
 * Scroll structure
 * size: 0x12
 */
typedef struct {
	uint16 flags;				// 0x00
	sint16 h_left;				// 0x02
	sint16 h_right;				// 0x04
	sint16 h_thumb_left;		// 0x06
	sint16 h_thumb_right;		// 0x08
	sint16 v_top;				// 0x0A
	sint16 v_bottom;			// 0x0C
	sint16 v_thumb_top;			// 0x0E
	sint16 v_thumb_bottom;		// 0x10
} rct_scroll;

/** 
 * Viewport focus structure.
 * size: 0xA
 * Use sprite.type to work out type.
 */
typedef struct{
	sint16 var_480;
	sint16 x; //0x482
	sint16 y; //0x484 & VIEWPORT_FOCUS_Y_MASK
	sint16 z; //0x486
	uint8 rotation;//0x488
	uint8 zoom;//0x489
} coordinate_focus;

// Type is viewport_target_sprite_id & 0x80000000 != 0
typedef struct{
	sint16 var_480;
	uint16 sprite_id; //0x482
	uint8 pad_484;
	uint8 type; //0x485 & VIEWPORT_FOCUS_TYPE_MASK
	uint16 pad_486; 
	uint8 rotation; //0x488
	uint8 zoom; //0x489
} sprite_focus;

#define VIEWPORT_FOCUS_TYPE_MASK 0xC0
enum{
	VIEWPORT_FOCUS_TYPE_COORDINATE = (1<<6),
	VIEWPORT_FOCUS_TYPE_SPRITE = (1<<7)
};
#define VIEWPORT_FOCUS_Y_MASK 0x3FFF


typedef struct{
	sint16 campaign_type;
	sint16 no_weeks; //0x482
	uint16 ride_id; //0x484
	uint32 pad_486;
} campaign_variables;

typedef struct{
	sint16 selected_ride_id; //0x480
	sint16 highlighted_ride_id; //0x482
	uint16 pad_484;
	uint16 pad_486;
	uint16 selected_ride_countdown; //488
} new_ride_variables;

typedef struct{
	sint16 var_480;
	sint16 var_482;
	uint16 var_484;
	uint16 var_486;
	uint16 var_488;
} news_variables;

typedef struct{
	sint16 rotation;
	sint16 var_482;
	uint16 var_484;
	uint16 var_486;
	uint16 var_488;
} map_variables;

typedef struct {
	sint16 view;
	sint32 var_482;
	sint32 var_486;
} ride_variables;

typedef struct {
	sint16 selected_scenery_id;
	sint16 hover_counter;
} scenery_variables;

typedef struct {
	uint16 var_480;
	uint16 var_482;
	uint16 var_484;
} track_list_variables;

typedef struct {
	uint16 var_480;
} error_variables;

/**
 * Window structure
 * size: 0x4C0
 */
typedef struct rct_window {
	uint32* event_handlers;		// 0x000
	rct_viewport* viewport;		// 0x004
	uint64 enabled_widgets;		// 0x008
	uint64 disabled_widgets;	// 0x010
	uint64 pressed_widgets;		// 0x018
	uint64 var_020;
	rct_widget* widgets;		// 0x028
	sint16 x;					// 0x02C
	sint16 y;					// 0x02E
	sint16 width;				// 0x030
	sint16 height;				// 0x032
	sint16 min_width;			// 0x034
	sint16 max_width;			// 0x036
	sint16 min_height;			// 0x038
	sint16 max_height;			// 0x03A
	rct_windownumber number;	// 0x03C
	uint16 flags;				// 0x03E
	rct_scroll scrolls[3];		// 0x040
	uint8 list_item_positions[1024];// 0x076
	uint16 no_list_items;			// 0x476 0 for no items
	sint16 pad_478;
	sint16 selected_list_item;		// 0x47A -1 for none selected
	sint16 pad_47C;
	sint16 pad_47E;
	union {
		coordinate_focus viewport_focus_coordinates;
		sprite_focus viewport_focus_sprite;
		campaign_variables campaign;
		new_ride_variables new_ride;
		news_variables news;
		map_variables map;
		ride_variables ride;
		scenery_variables scenery;
		track_list_variables track_list;
		error_variables error;
	};
	sint16 page;					// 0x48A
	sint16 var_48C;
	sint16 frame_no;				// 0x48E updated every tic for motion in windows sprites
	uint16 list_information_type;	// 0x490 0 for none, Used as current position of marquee in window_peep
	sint16 var_492;
	uint32 var_494;
	uint8 var_498[0x14];
	sint16 selected_tab;			// 0x4AC
	sint16 var_4AE;
	sint16 viewport_target_sprite;	// 0x4B0 viewport target sprite?
	sint16 saved_view_x;			// 0x4B2
	sint16 saved_view_y; 			// 0x4B4
	rct_windowclass classification;	// 0x4B6
	uint8 pad_4B7;
	sint8 var_4B8;
	sint8 var_4B9;
	uint8 colours[6];			// 0x4BA
} rct_window;

#define RCT_WINDOW_RIGHT(w) (w->x + w->width)
#define RCT_WINDOW_BOTTOM(w) (w->y + w->height)

typedef enum {
	WE_CLOSE = 0,
	WE_MOUSE_UP = 1,
	WE_RESIZE = 2,
	WE_MOUSE_DOWN = 3,
	WE_DROPDOWN = 4,
	WE_UNKNOWN_05 = 5, 
	// Unknown 05: Used to update tabs that are not being animated
	// see window_peep. When the overview tab is not highlighted the
	// items being carried such as hats/balloons still need to be shown
	// and removed. Probably called after anything that affects items
	// being carried.
	WE_UPDATE = 6,
	WE_UNKNOWN_07 = 7,
	WE_UNKNOWN_08 = 8,
	WE_TOOL_UPDATE = 9,
	WE_TOOL_DOWN = 10,
	WE_TOOL_DRAG = 11,
	WE_TOOL_UP = 12,
	WE_TOOL_ABORT = 13,
	WE_UNKNOWN_0E = 14,
	WE_SCROLL_GETSIZE = 15,
	WE_SCROLL_MOUSEDOWN = 16,
	WE_UNKNOWN_11 = 17,
	WE_SCROLL_MOUSEOVER = 18,
	WE_TEXT_INPUT = 19,
	WE_UNKNOWN_14 = 20,
	WE_UNKNOWN_15 = 21, // scroll mouse move?
	WE_TOOLTIP = 22,
	WE_UNKNOWN_17 = 23, // tooltip related
	WE_UNKNOWN_18 = 24,
	WE_INVALIDATE = 25,
	WE_PAINT = 26,
	WE_SCROLL_PAINT = 27,
} WINDOW_EVENTS;

typedef enum {
	/*
	WF_TIMEOUT_SHL = 0,
	WF_TIMEOUT_MASK = 7,
	WF_DRAGGING = 1 << 3,
	WF_SCROLLER_UP = 1 << 4,
	WF_SCROLLER_DOWN = 1 << 5,
	WF_SCROLLER_MIDDLE = 1 << 6,
	WF_DISABLE_VP_SCROLL = 1 << 9,
	*/

	WF_STICK_TO_BACK = (1 << 0),
	WF_STICK_TO_FRONT = (1 << 1),
	WF_2 = (1 << 2),
	WF_SCROLLING_TO_LOCATION = (1 << 3),
	WF_TRANSPARENT = (1 << 4),
	WF_5 = (1 << 5),
	WF_7 = (1 << 7),
	WF_RESIZABLE = (1 << 8),
	WF_9 = (1 << 9),
	WF_10 = (1 << 10),
	WF_WHITE_BORDER_ONE = (1 << 12),
	WF_WHITE_BORDER_MASK = (1 << 12) | (1 << 13),
} WINDOW_FLAGS;

enum SCROLL_FLAGS {
	HSCROLLBAR_VISIBLE = (1 << 0),
	HSCROLLBAR_THUMB_PRESSED = (1 << 1),
	HSCROLLBAR_LEFT_PRESSED = (1 << 2),
	HSCROLLBAR_RIGHT_PRESSED = (1 << 3),
	VSCROLLBAR_VISIBLE = (1 << 4),
	VSCROLLBAR_THUMB_PRESSED = (1 << 5),
	VSCROLLBAR_UP_PRESSED = (1 << 6),
	VSCROLLBAR_DOWN_PRESSED = (1 << 7),
};

#define SCROLLBAR_SIZE			16

enum {
	SCROLL_PART_NONE = -1,
	SCROLL_PART_VIEW = 0,
	SCROLL_PART_HSCROLLBAR_LEFT = 1,
	SCROLL_PART_HSCROLLBAR_RIGHT = 2,
	SCROLL_PART_HSCROLLBAR_LEFT_TROUGH = 3,
	SCROLL_PART_HSCROLLBAR_RIGHT_TROUGH = 4,
	SCROLL_PART_HSCROLLBAR_THUMB = 5,
	SCROLL_PART_VSCROLLBAR_TOP = 6,
	SCROLL_PART_VSCROLLBAR_BOTTOM = 7,
	SCROLL_PART_VSCROLLBAR_TOP_TROUGH = 8,
	SCROLL_PART_VSCROLLBAR_BOTTOM_TROUGH = 9,
	SCROLL_PART_VSCROLLBAR_THUMB = 10,
};

enum {
	INPUT_STATE_RESET = 0,
	INPUT_STATE_NORMAL = 1,
	INPUT_STATE_WIDGET_PRESSED = 2,
	INPUT_STATE_POSITIONING_WINDOW = 3,
	INPUT_STATE_VIEWPORT_RIGHT = 4,
	INPUT_STATE_DROPDOWN_ACTIVE = 5,
	INPUT_STATE_VIEWPORT_LEFT = 6,
	INPUT_STATE_SCROLL_LEFT = 7,
	INPUT_STATE_RESIZING = 8,
};

enum {
	WC_MAIN_WINDOW = 0,
	WC_TOP_TOOLBAR = 1,
	WC_BOTTOM_TOOLBAR = 2,
	WC_TOOLTIP = 5,
	WC_DROPDOWN = 6,
	WC_ABOUT = 8,
	WC_PUBLISHER_CREDITS = 9,
	WC_MUSIC_CREDITS = 10,
	WC_ERROR = 11,
	WC_RIDE = 12,
	WC_RIDE_CONSTRUCTION = 13,
	WC_SAVE_PROMPT = 14,
	WC_RIDE_LIST = 15,
	WC_CONSTRUCT_RIDE = 16,
	WC_DEMOLISH_RIDE_PROMPT = 17,
	WC_SCENERY = 18,
	WC_OPTIONS = 19,
	WC_FOOTPATH = 20,
	WC_LAND = 21,
	WC_WATER = 22,
	WC_PEEP = 23,
	WC_GUEST_LIST = 24,
	WC_STAFF_LIST = 25,
	WC_FIRE_PROMPT = 26,
	WC_PARK_INFORMATION = 27,
	WC_FINANCES = 28,
	WC_TITLE_MENU = 29,
	WC_TITLE_EXIT = 30,
	WC_RECENT_NEWS = 31,
	WC_SCENARIO_SELECT = 32,
	WC_TRACK_DESIGN_LIST = 33,
	WC_TRACK_DESIGN_PLACE = 34,
	WC_NEW_CAMPAIGN = 35,
	WC_KEYBOARD_SHORTCUT_LIST = 36,
	WC_CHANGE_KEYBOARD_SHORTCUT = 37,
	WC_MAP = 38,
	WC_TITLE_LOGO = 39,
	WC_BANNER = 40,
	WC_MAP_TOOLTIP = 41,
	WC_EDITOR_OBJECT_SELECTION = 42,
	WC_EDITOR_INVENTION_LIST = 43,
	WC_EDITOR_SCENARIO_OPTIONS = 45,
	WC_EDTIOR_OBJECTIVE_OPTIONS = 46,
	WC_47,
	WC_48,
	WC_CLEAR_SCENERY = 50,
	WC_MANAGE_TRACK_DESIGN = 89,
	WC_CHEATS = 110,
	WC_RESEARCH = 111,
	WC_VIEWPORT = 112,
	WC_TEXTINPUT = 113
} WINDOW_CLASS;

enum PROMPT_MODE {
	PM_SAVE_BEFORE_LOAD = 0,
	PM_SAVE_BEFORE_QUIT,
	PM_SAVE_BEFORE_QUIT2,
	PM_QUIT
};

// rct2: 0x01420078
extern rct_window* g_window_list;

// rct2: 0x00F635EE
extern ride_list_item _window_track_list_item;

void window_dispatch_update_all();
void window_update_all();
rct_window *window_create(int x, int y, int width, int height, uint32 *event_handlers, rct_windowclass cls, uint16 flags);
rct_window *window_create_auto_pos(int width, int height, uint32 *event_handlers, rct_windowclass cls, uint16 flags);
void window_close(rct_window *window);
void window_close_by_class(rct_windowclass cls);
void window_close_by_number(rct_windowclass cls, rct_windownumber number);
void window_close_top();
void window_close_all();
rct_window *window_find_by_class(rct_windowclass cls);
rct_window *window_find_by_number(rct_windowclass cls, rct_windownumber number);
rct_window *window_find_from_point(int x, int y);
int window_find_widget_from_point(rct_window *w, int x, int y);
void window_invalidate(rct_window *window);
void window_invalidate_by_class(rct_windowclass cls);
void window_invalidate_by_number(rct_windowclass cls, rct_windownumber number);
void widget_invalidate(rct_window *w, int widgetIndex);
void widget_invalidate_by_class(rct_windowclass cls, int widgetIndex);
void widget_invalidate_by_number(rct_windowclass cls, rct_windownumber number, int widgetIndex);
void window_init_scroll_widgets(rct_window *w);
void window_update_scroll_widgets(rct_window *w);
int window_get_scroll_data_index(rct_window *w, int widget_index);
int window_get_scroll_size(rct_window *w, int scrollIndex, int *width, int *height);

rct_window *window_bring_to_front(rct_window *w);
rct_window *window_bring_to_front_by_class(rct_windowclass cls);
rct_window *window_bring_to_front_by_number(rct_windowclass cls, rct_windownumber number);

void window_push_others_right(rct_window *w);
void window_push_others_below(rct_window *w1);

rct_window *window_get_main();

void window_scroll_to_viewport(rct_window *w);
void window_scroll_to_location(rct_window *w, int x, int y, int z);
void window_rotate_camera(rct_window *w);
void window_zoom_in(rct_window *w);
void window_zoom_out(rct_window *w);

void window_show_textinput(rct_window *w, int widgetIndex, uint16 title, uint16 text, int value);

void window_draw(rct_window *w, int left, int top, int right, int bottom);
void window_draw_widgets(rct_window *w, rct_drawpixelinfo *dpi);
void window_draw_viewport(rct_drawpixelinfo *dpi, rct_window *w);

void window_set_position(rct_window *w, int x, int y);
void window_move_position(rct_window *w, int dx, int dy);
void window_resize(rct_window *w, int dw, int dh);
void window_set_resize(rct_window *w, int minWidth, int minHeight, int maxWidth, int maxHeight);

int tool_set(rct_window *w, int widgetIndex, int tool);
void tool_cancel();

void window_close_construction_windows();

void window_update_viewport_ride_music();

// Open window functions
void window_main_open();
void window_resize_gui(int width, int height);
void window_resize_gui_scenario_editor(int width, int height);
void window_game_top_toolbar_open();
void window_game_bottom_toolbar_open();
void window_about_open();
void window_footpath_open();
void window_save_prompt_open();
void window_title_menu_open();
void window_title_exit_open();
void window_title_logo_open();
void window_news_open();
void window_scenarioselect_open();
void window_track_list_open(ride_list_item item);
void window_clear_scenery_open();
void window_land_open();
void window_water_open();
void window_staff_list_open();
void window_guest_list_open();
void window_map_open();
void window_options_open();
void window_shortcut_keys_open();
void window_shortcut_change_open(int selected_key);
void window_guest_open(rct_peep* peep);
void window_staff_open(rct_peep* peep);
void window_staff_fire_prompt_open(rct_peep* peep);
void window_park_awards_open();
void window_park_entrance_open();
void window_park_guests_open();
void window_park_objective_open();
void window_park_rating_open();
void window_finances_open();
void window_finances_research_open();
void window_new_campaign_open(sint16 campaignType);
rct_window *window_ride_main_open(int rideIndex);
rct_window *window_ride_open_station(int rideIndex, int stationIndex);
rct_window *window_ride_open_track(rct_map_element *mapElement);
rct_window *window_ride_open_vehicle(rct_vehicle *vehicle);
void window_ride_demolish_prompt_open(int rideIndex);
void window_ride_construct(rct_window *w);
void window_ride_list_open();
rct_window * window_construction_open();
void window_track_place_open();
void window_new_ride_open();
void window_banner_open(rct_windownumber number);
void window_sign_open(rct_windownumber number);
void window_sign_small_open(rct_windownumber number);
void window_cheats_open();
void window_research_open();
void window_scenery_open();
void window_music_credits_open();
void window_publisher_credits_open();
void window_track_manage_open();
void window_viewport_open();
void window_text_input_open(rct_window* call_w, int call_widget, rct_string_id title, rct_string_id description, rct_string_id existing_text, uint32 existing_args);

void window_guest_list_init_vars_a();
void window_guest_list_init_vars_b();
void window_bubble_list_item(rct_window* w, int item_position);

void window_align_tabs( rct_window *w, uint8 start_tab_id, uint8 end_tab_id );

void window_new_ride_init_vars();
void window_new_ride_focus(ride_list_item rideItem);

void window_map_tooltip_update_visibility();

void window_staff_list_init_vars();

void window_event_helper(rct_window* w, short widgetIndex, WINDOW_EVENTS event);

void sub_6EA73F();

void window_move_and_snap(rct_window *w, int newWindowX, int newWindowY, int snapProximity);
int window_can_resize(rct_window *w);

#ifdef _MSC_VER
	#define window_get_register(w)														\
		__asm mov w, esi

	#define window_widget_get_registers(w, widgetIndex)									\
		__asm mov widgetIndex, dx														\
		__asm mov w, esi

	#define window_dropdown_get_registers(w, widgetIndex, dropdownIndex)				\
		__asm mov dropdownIndex, ax														\
		__asm mov widgetIndex, dx														\
		__asm mov w, esi
	
	#define window_text_input_get_registers(w, widgetIndex, result, text)					\
		__asm mov widgetIndex, dx														\
		__asm mov result, cl																\
		__asm mov w, esi																\
		__asm mov text, edi

	#define window_scrollmouse_get_registers(w, x, y)									\
		__asm mov x, cx																	\
		__asm mov y, dx																	\
		__asm mov w, esi

	#define window_tool_get_registers(w, widgetIndex, x, y)								\
		__asm mov x, ax																	\
		__asm mov y, bx																	\
		__asm mov widgetIndex, dx														\
		__asm mov w, esi

	#define window_textinput_get_registers(w, widgetIndex, result, text)				\
		__asm mov result, cl															\
		__asm mov widgetIndex, dx														\
		__asm mov w, esi																\
		__asm mov text, edi

	#define window_paint_get_registers(w, dpi)											\
		__asm mov w, esi																\
		__asm mov dpi, edi
#else
	#define window_get_register(w)														\
		__asm__ ( "mov %["#w"], esi " : [w] "+m" (w) );

	#define window_widget_get_registers(w, widgetIndex)									\
		__asm__ ( "mov %["#widgetIndex"], dx " : [widgetIndex] "+m" (widgetIndex) );	\
		__asm__ ( "mov %["#w"], esi " : [w] "+m" (w) );

	#define window_dropdown_get_registers(w, widgetIndex, dropdownIndex)				\
		__asm__ ( "mov %["#dropdownIndex"], ax " : [dropdownIndex] "+m" (dropdownIndex) );	\
		__asm__ ( "mov %["#widgetIndex"], dx " : [widgetIndex] "+m" (widgetIndex) );		\
		__asm__ ( "mov %["#w"], esi " : [w] "+m" (w) );

	#define window_text_input_get_registers(w, widgetIndex, result, text)					\
		__asm__ ( "mov %[_cl], cl " : [_cl] "+m" (result) );								\
		__asm__ ( "mov %[widgetIndex], dx " : [widgetIndex] "+m" (widgetIndex) );		\
		__asm__ ( "mov %[w], esi " : [w] "+m" (w) );									\
		__asm__ ( "mov %[text], edi " : [text] "+m" (text) );

	#define window_scrollmouse_get_registers(w, x, y)									\
		__asm__ ( "mov %["#x"], cx " : [x] "+m" (x) );										\
		__asm__ ( "mov %["#y"], dx " : [y] "+m" (y) );										\
		__asm__ ( "mov %["#w"], esi " : [w] "+m" (w) );

	#define window_tool_get_registers(w, widgetIndex, x, y)								\
		__asm__ ( "mov %["#x"], ax " : [x] "+m" (x) );										\
		__asm__ ( "mov %["#y"], bx " : [y] "+m" (y) );										\
		__asm__ ( "mov %["#widgetIndex"], dx " : [widgetIndex] "+m" (widgetIndex) );		\
		__asm__ ( "mov %["#w"], esi " : [w] "+m" (w) );

	#define window_textinput_get_registers(w, widgetIndex, result, text)				\
		__asm__ ( "mov %[result], cl " : [result] "+m" (result) );						\
		__asm__ ( "mov %[widgetIndex], dx " : [widgetIndex] "+m" (widgetIndex) );		\
		__asm__ ( "mov %[w], esi " : [w] "+m" (w) );									\
		__asm__ ( "mov %[text], edi " : [text] "+m" (text) );

	#define window_paint_get_registers(w, dpi)											\
		__asm__ ( "mov %["#w"], esi " : [w] "+m" (w) );									\
		__asm__ ( "mov %["#dpi"], edi " : [dpi] "+m" (dpi) );
#endif

#endif
