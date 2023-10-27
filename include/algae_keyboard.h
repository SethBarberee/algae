#ifndef ALGAE_KEYBOARD_H
#define ALGAE_KEYBOARD_H

struct algae_keyboard {
	struct wl_list link;
	struct algae_server *server;
	struct wlr_keyboard *wlr_keyboard;

	struct wl_listener modifiers;
	struct wl_listener key;
	struct wl_listener destroy;
};

void server_new_keyboard(struct algae_server *server, struct wlr_input_device *device);

#endif // ALGAE_KEYBOARD_H
