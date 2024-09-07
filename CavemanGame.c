
int entry(int argc, char **argv) {
	
	// This is how we (optionally) configure the window.
	// To see all the settable window properties, ctrl+f "struct Os_Window" in os_interface.c
	window.title = STR("Caveman Game");
	window.clear_color = hex_to_rgba(0x4b692fff);
	
Gfx_Image *player = load_image_from_disk(fixed_string("/assets/Caveman.png"), get_heap_allocator());
assert(player != NULL, "Bad shit happened");

	float64 last_time = os_get_elapsed_seconds();
	while (!window.should_close) {

		reset_temporary_storage();
		
		float64 now = os_get_elapsed_seconds();
		if ((int)now != (int)last_time) log("%.2f FPS\n%.2fms", 1.0/(now-last_time), (now-last_time)*1000);
		last_time = now;
		
		Matrix4 xform = m4_scalar(1.0);
		xform         = m4_rotate_z(xform, (f32)now);
		xform         = m4_translate(xform, v3(-.25f, -.25f, 0));
		draw_image_xform(player, xform, v2(.5f, .5f), COLOR_GREEN);

		os_update();
		
		if (is_key_just_released(KEY_ESCAPE)) {
			window.should_close = true;
		}

		gfx_update();
	}

	return 0;
}