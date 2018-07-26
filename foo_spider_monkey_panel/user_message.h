#pragma once

enum t_user_message
{
	CALLBACK_UWM_ON_ALWAYS_ON_TOP_CHANGED = WM_USER + 100,
	CALLBACK_UWM_ON_COLOURS_CHANGED,
	CALLBACK_UWM_ON_CURSOR_FOLLOW_PLAYBACK_CHANGED,
    CALLBACK_UWM_ON_DRAG_DROP,
    CALLBACK_UWM_ON_DRAG_ENTER,
    CALLBACK_UWM_ON_DRAG_LEAVE,
    CALLBACK_UWM_ON_DRAG_OVER,
	CALLBACK_UWM_ON_DSP_PRESET_CHANGED,
	CALLBACK_UWM_ON_FONT_CHANGED,
	CALLBACK_UWM_ON_GET_ALBUM_ART_DONE,
	CALLBACK_UWM_ON_ITEM_FOCUS_CHANGE,
	CALLBACK_UWM_ON_ITEM_PLAYED,
	CALLBACK_UWM_ON_LIBRARY_ITEMS_ADDED,
	CALLBACK_UWM_ON_LIBRARY_ITEMS_CHANGED,
	CALLBACK_UWM_ON_LIBRARY_ITEMS_REMOVED,
	CALLBACK_UWM_ON_LOAD_IMAGE_DONE,
	CALLBACK_UWM_ON_MAIN_MENU,
	CALLBACK_UWM_ON_METADB_CHANGED,
	CALLBACK_UWM_ON_NOTIFY_DATA,
	CALLBACK_UWM_ON_OUTPUT_DEVICE_CHANGED,
	CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO,
	CALLBACK_UWM_ON_PLAYBACK_DYNAMIC_INFO_TRACK,
	CALLBACK_UWM_ON_PLAYBACK_EDITED,
	CALLBACK_UWM_ON_PLAYBACK_FOLLOW_CURSOR_CHANGED,
	CALLBACK_UWM_ON_PLAYBACK_NEW_TRACK,
	CALLBACK_UWM_ON_PLAYBACK_ORDER_CHANGED,
	CALLBACK_UWM_ON_PLAYBACK_PAUSE,
	CALLBACK_UWM_ON_PLAYBACK_QUEUE_CHANGED,
	CALLBACK_UWM_ON_PLAYBACK_SEEK,
	CALLBACK_UWM_ON_PLAYBACK_STARTING,
	CALLBACK_UWM_ON_PLAYBACK_STOP,
	CALLBACK_UWM_ON_PLAYBACK_TIME,
	CALLBACK_UWM_ON_PLAYLIST_ITEM_ENSURE_VISIBLE,
	CALLBACK_UWM_ON_PLAYLIST_ITEMS_ADDED,
	CALLBACK_UWM_ON_PLAYLIST_ITEMS_REMOVED,
	CALLBACK_UWM_ON_PLAYLIST_ITEMS_REORDERED,
	CALLBACK_UWM_ON_PLAYLIST_ITEMS_SELECTION_CHANGE,
	CALLBACK_UWM_ON_PLAYLIST_STOP_AFTER_CURRENT_CHANGED,
	CALLBACK_UWM_ON_PLAYLIST_SWITCH,
	CALLBACK_UWM_ON_PLAYLISTS_CHANGED,
	CALLBACK_UWM_ON_REPLAYGAIN_MODE_CHANGED,
	CALLBACK_UWM_ON_SELECTION_CHANGED,
	CALLBACK_UWM_ON_VOLUME_CHANGE,
	UWM_FIND_TEXT_CHANGED,
	UWM_KEYDOWN,
	UWM_REFRESHBK,
	UWM_RELOAD,
	UWM_SCRIPT_ERROR,
	UWM_SCRIPT_TERM,
	UWM_SHOW_CONFIGURE,
	UWM_SHOW_PROPERTIES,
	UWM_SIZE,
	UWM_SIZE_LIMIT_CHANGED,
	UWM_TIMER,
};
