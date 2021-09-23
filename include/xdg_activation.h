#ifndef XDG_ACTIVATION_H
#define XDG_ACTIVATION_H

#include <wlr/types/wlr_xdg_activation_v1.h>

void xdg_activation_v1_handle_request_activate(struct wl_listener *listener,
		void *data);

#endif // XDG_ACTIVATION_H
