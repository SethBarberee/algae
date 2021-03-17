#define _POSIX_C_SOURCE 200112L
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/backend/session.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/util/log.h>
#include <xkbcommon/xkbcommon.h>

struct algae_state {
	struct wl_display *display;
	struct wl_listener new_output;
	struct wl_listener new_input;
	struct timespec last_frame;
	float color[4];
	int dec;
};

struct algae_output {
	struct algae_state *algae;
	struct wlr_output *output;
	struct wl_listener frame;
	struct wl_listener destroy;
};

struct algae_keyboard {
	struct algae_state *algae;
	struct wlr_input_device *device;
	struct wl_listener key;
	struct wl_listener destroy;
};

static void output_frame_notify(struct wl_listener *listener, void *data) {
	struct algae_output *algae_output =
		wl_container_of(listener, algae_output, frame);
	struct algae_state *algae = algae_output->algae;
	struct wlr_output *wlr_output = algae_output->output;

	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);

	long ms = (now.tv_sec - algae->last_frame.tv_sec) * 1000 +
		(now.tv_nsec - algae->last_frame.tv_nsec) / 1000000;
	int inc = (algae->dec + 1) % 3;

	algae->color[inc] += ms / 2000.0f;
	algae->color[algae->dec] -= ms / 2000.0f;

	if (algae->color[algae->dec] < 0.0f) {
		algae->color[inc] = 1.0f;
		algae->color[algae->dec] = 0.0f;
		algae->dec = inc;
	}

	wlr_output_attach_render(wlr_output, NULL);

	struct wlr_renderer *renderer =
		wlr_backend_get_renderer(wlr_output->backend);
	wlr_renderer_begin(renderer, wlr_output->width, wlr_output->height);
	wlr_renderer_clear(renderer, algae->color);
	wlr_renderer_end(renderer);

	wlr_output_commit(wlr_output);
	algae->last_frame = now;
}

static void output_remove_notify(struct wl_listener *listener, void *data) {
	struct algae_output *algae_output =
		wl_container_of(listener, algae_output, destroy);
	wlr_log(WLR_DEBUG, "Output removed");
	wl_list_remove(&algae_output->frame.link);
	wl_list_remove(&algae_output->destroy.link);
	free(algae_output);
}

static void new_output_notify(struct wl_listener *listener, void *data) {
	struct wlr_output *output = data;
	struct algae_state *algae =
		wl_container_of(listener, algae, new_output);
	struct algae_output *algae_output =
		calloc(1, sizeof(struct algae_output));

	struct wlr_output_mode *mode = wlr_output_preferred_mode(output);
	if (mode != NULL) {
		wlr_output_set_mode(output, mode);
	}
	algae_output->output = output;
	algae_output->algae = algae;
	wl_signal_add(&output->events.frame, &algae_output->frame);
	algae_output->frame.notify = output_frame_notify;
	wl_signal_add(&output->events.destroy, &algae_output->destroy);
	algae_output->destroy.notify = output_remove_notify;

	wlr_output_commit(algae_output->output);
}

static void keyboard_key_notify(struct wl_listener *listener, void *data) {
	struct algae_keyboard *keyboard = wl_container_of(listener, keyboard, key);
	struct algae_state *algae = keyboard->algae;
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
	struct algae_state *algae = wl_container_of(listener, algae, new_input);
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
	struct wl_display *display = wl_display_create();
	struct algae_state state = {
		.color = { 1.0, 0.0, 0.0, 1.0 },
		.dec = 0,
		.last_frame = { 0 },
		.display = display
	};
	struct wlr_backend *backend = wlr_backend_autocreate(display, NULL);
	if (!backend) {
		exit(1);
	}
	wl_signal_add(&backend->events.new_output, &state.new_output);
	state.new_output.notify = new_output_notify;
	wl_signal_add(&backend->events.new_input, &state.new_input);
	state.new_input.notify = new_input_notify;
	clock_gettime(CLOCK_MONOTONIC, &state.last_frame);

	if (!wlr_backend_start(backend)) {
		wlr_log(WLR_ERROR, "Failed to start backend");
		wlr_backend_destroy(backend);
		exit(1);
	}
	wl_display_run(display);
	wl_display_destroy(display);
}
