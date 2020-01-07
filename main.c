#include <wayland-server-core.h>

int main(int argc, const char *argv[])
{
   struct wl_display *display = wl_display_create();
   wl_display_run(display);
}
