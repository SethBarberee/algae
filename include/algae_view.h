#ifndef  ALGAE_VIEW_H
#define ALGAE_VIEW_H 

struct algae_view {
	struct wl_list link;
	struct algae_server *server;
	struct wlr_xdg_surface *xdg_surface;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_maximize;
	struct wl_listener set_title;
	bool mapped;
	int x, y;
};

#endif // ALGAE_VIEW_H

