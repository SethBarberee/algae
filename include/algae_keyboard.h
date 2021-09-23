#ifndef ALGAE_KEYBOARD_H
#define ALGAE_KEYBOARD_H

struct algae_keyboard {
	struct wl_list link;
	struct algae_server *server;
	struct wlr_input_device *device;

	struct wl_listener modifiers;
	struct wl_listener key;
};

#endif // ALGAE_KEYBOARD_H
