#ifndef ALGAE_OUTPUT_H
#define ALGAE_OUTPUT_H 

struct algae_output {
	struct wl_list link;
	struct algae_server *server;
	struct wlr_output *wlr_output;
	struct wl_listener frame;
	struct wl_listener request_state;
	struct wl_listener destroy;
};

#endif // ALGAE_OUTPUT_H
