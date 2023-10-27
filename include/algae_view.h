#ifndef ALGAE_VIEW_H
#define ALGAE_VIEW_H 

struct algae_view {
	struct wl_list link;
	struct algae_server *server;
	struct wlr_xdg_toplevel *xdg_toplevel;
	struct wlr_scene_tree *scene_tree;

	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	struct wl_listener request_maximize;
	struct wl_listener request_fullscreen;
};

void focus_view(struct algae_view *view, struct wlr_surface *surface);

#endif // ALGAE_VIEW_H

