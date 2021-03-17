#define _POSIX_C_SOURCE 200112L
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/backend/session.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_control_v1.h>
#include <wlr/types/wlr_export_dmabuf_v1.h>
#include <wlr/types/wlr_gamma_control_v1.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

struct algae_server {
	struct wl_display *display;
        struct wlr_backend *backend;
        struct wlr_compositor *compositor;

	struct wl_listener new_output;
	struct wl_listener new_input;

        struct wl_list outputs; // algae_output::link
};

struct algae_output {
	struct algae_server *server;
	struct wlr_output *output;
	struct wl_listener frame;
	struct wl_listener destroy;
	struct timespec last_frame;

        struct wl_list link;
};

struct algae_keyboard {
	struct algae_server *algae;
	struct wlr_input_device *device;
	struct wl_listener key;
	struct wl_listener destroy;
};


static void output_frame_notify(struct wl_listener *listener, void *data) {
	struct algae_output *algae_output =
		wl_container_of(listener, algae_output, frame);
	struct wlr_output *wlr_output = algae_output->output;
        struct algae_server *algae_server = algae_output->server;

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	long ms = (now.tv_sec - algae_output->last_frame.tv_sec) * 1000 +
		(now.tv_nsec - algae_output->last_frame.tv_nsec) / 1000000;


	wlr_output_attach_render(wlr_output, NULL);

	struct wlr_renderer *renderer =
		wlr_backend_get_renderer(wlr_output->backend);
	wlr_renderer_begin(renderer, wlr_output->width, wlr_output->height);

        float color[4] = { 0.4f, 0.4f, 0.4f, 1.0f };
	wlr_renderer_clear(renderer, color);

	wlr_renderer_end(renderer);

	wlr_output_commit(wlr_output);
	algae_output->last_frame = now;
}

static void output_remove_notify(struct wl_listener *listener, void *data) {
	struct algae_output *algae_output =
		wl_container_of(listener, algae_output, destroy);
	wlr_log(WLR_DEBUG, "Output removed");
	wl_list_remove(&algae_output->link);
	wl_list_remove(&algae_output->frame.link);
	wl_list_remove(&algae_output->destroy.link);
	free(algae_output);
}

static void new_output_notify(struct wl_listener *listener, void *data) {
	struct wlr_output *output = data;
	struct algae_server *algae =
		wl_container_of(listener, algae, new_output);
	struct algae_output *algae_output =
		calloc(1, sizeof(struct algae_output));

	clock_gettime(CLOCK_MONOTONIC, &algae_output->last_frame);

	struct wlr_output_mode *mode = wlr_output_preferred_mode(output);
	if (mode != NULL) {
		wlr_output_set_mode(output, mode);
	}
	algae_output->output = output;
	algae_output->server = algae;

        wl_list_insert(&algae->outputs, &algae_output->link);

	wl_signal_add(&output->events.frame, &algae_output->frame);
	algae_output->frame.notify = output_frame_notify;
	wl_signal_add(&output->events.destroy, &algae_output->destroy);
	algae_output->destroy.notify = output_remove_notify;

        wlr_output_create_global(algae_output->output); // Add this output to the global

	wlr_output_commit(algae_output->output);
}

static void keyboard_key_notify(struct wl_listener *listener, void *data) {
	struct algae_keyboard *keyboard = wl_container_of(listener, keyboard, key);
	struct algae_server *algae = keyboard->algae;
	struct wlr_event_keyboard_key *event = data;
	uint32_t keycode = event->keycode + 8;
	const xkb_keysym_t *syms;
	int nsyms = xkb_state_key_get_syms(keyboard->device->keyboard->xkb_state,
			keycode, &syms);
	for (int i = 0; i < nsyms; i++) {
		xkb_keysym_t sym = syms[i];
		if (sym == XKB_KEY_Escape) {
			wl_display_terminate(algae->display);
		}
	}
}

static void keyboard_destroy_notify(struct wl_listener *listener, void *data) {
	struct algae_keyboard *keyboard =
		wl_container_of(listener, keyboard, destroy);
	wl_list_remove(&keyboard->destroy.link);
	wl_list_remove(&keyboard->key.link);
	free(keyboard);
}

static void new_input_notify(struct wl_listener *listener, void *data) {
	struct wlr_input_device *device = data;
	struct algae_server *algae = wl_container_of(listener, algae, new_input);
	switch (device->type) {
	case WLR_INPUT_DEVICE_KEYBOARD:;
		struct algae_keyboard *keyboard =
			calloc(1, sizeof(struct algae_keyboard));
		keyboard->device = device;
		keyboard->algae = algae;
		wl_signal_add(&device->events.destroy, &keyboard->destroy);
		keyboard->destroy.notify = keyboard_destroy_notify;
		wl_signal_add(&device->keyboard->events.key, &keyboard->key);
		keyboard->key.notify = keyboard_key_notify;
		struct xkb_rule_names rules = { 0 };
		rules.rules = getenv("XKB_DEFAULT_RULES");
		rules.model = getenv("XKB_DEFAULT_MODEL");
		rules.layout = getenv("XKB_DEFAULT_LAYOUT");
		rules.variant = getenv("XKB_DEFAULT_VARIANT");
		rules.options = getenv("XKB_DEFAULT_OPTIONS");
		struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
		if (!context) {
			wlr_log(WLR_ERROR, "Failed to create XKB context");
			exit(1);
		}
		struct xkb_keymap *keymap = xkb_map_new_from_names(context, &rules,
			XKB_KEYMAP_COMPILE_NO_FLAGS);
		if (!keymap) {
			wlr_log(WLR_ERROR, "Failed to create XKB keymap");
			exit(1);
		}
		wlr_keyboard_set_keymap(device->keyboard, keymap);
		xkb_keymap_unref(keymap);
		xkb_context_unref(context);
		break;
	default:
		break;
	}
}

int main(void) {
	wlr_log_init(WLR_DEBUG, NULL);
        struct algae_server server;

	server.display = wl_display_create();
	assert(server.display);

	server.backend = wlr_backend_autocreate(server.display, NULL);
	assert(server.backend);

        wl_list_init(&server.outputs);

	wl_signal_add(&server.backend->events.new_output, &server.new_output);
	server.new_output.notify = new_output_notify;

	wl_signal_add(&server.backend->events.new_input, &server.new_input);
	server.new_input.notify = new_input_notify;

	// Avoid using "wayland-0" as display socket
	char name_candidate[16];
        const char *socket;
	for (int i = 1; i <= 32; ++i) {
		sprintf(name_candidate, "wayland-%d", i);
		if (wl_display_add_socket(server.display, name_candidate) >= 0) {
			socket = strdup(name_candidate);
			break;
		}
	}

	if (!wlr_backend_start(server.backend)) {
		wlr_log(WLR_ERROR, "Failed to start backend");
		wlr_backend_destroy(server.backend);
		exit(1);
	}

        printf("Running compositor on wayland display '%s'\n", socket);
        setenv("WAYLAND_DISPLAY", socket, true);

        wl_display_init_shm(server.display);
	wlr_export_dmabuf_manager_v1_create(server.display);
	wlr_screencopy_manager_v1_create(server.display);
	wlr_data_control_manager_v1_create(server.display);
        wlr_gamma_control_manager_v1_create(server.display);

        server.compositor = wlr_compositor_create(server.display,
                wlr_backend_get_renderer(server.backend));

        wlr_xdg_shell_create(server.display);

	wl_display_run(server.display);
	wl_display_destroy(server.display);
}
