
int entry(int argc, char **argv) {
	
	// This is how we (optionally) configure the window.
	// To see all the settable window properties, ctrl+f "struct Os_Window" in os_interface.c
	window.title = STR("Caveman Game");
	window.width = 1280;
	window.height = 720;
	window.x = 200;
	window.y = 200;
	window.clear_color = hex_to_rgba(0x4b692fff);
	
	// loads player image
	Gfx_Image* player = load_image_from_disk(fixed_string("assets/player.png"), get_heap_allocator());
	assert(player, "Failed loading player.png");
	
	float64 last_time = os_get_elapsed_seconds();
	Vector2 player_pos = v2(0, 0); // initial player position

	// game loop
	while (!window.should_close) {
		// should close the window if escape is pressed
		if (is_key_just_pressed(KEY_ESCAPE)) {
			window.should_close = true;
		}

		// player movement
		Vector2 input_axis = v2(0, 0);
		if (is_key_down('A')) {
			input_axis.x -= 1.0;
		}
		if (is_key_down('D')) {
			input_axis.x += 1.0;
		}
		if (is_key_down('S')) {
			input_axis.y -= 1.0;
		}
		if (is_key_down('W')) {
			input_axis.y += 1.0;
		}
		// normalize input axis
		input_axis = v2_normalize(input_axis);

		// player_pos = player_pos + (input_axis * 10.0);
		player_pos = v2_add(player_pos, v2_mulf(input_axis, 0.1));

		reset_temporary_storage();
		
		// records the time passed since the last frame
		float64 now = os_get_elapsed_seconds();
		float64 delta = now - last_time;
		if ((int)now != (int)last_time) log("%.2f FPS\n%.2fms", 1.0/(now-last_time), (now-last_time)*1000);
		last_time = now;
		
		// draw player
		Matrix4 xform = m4_scalar(1.0);
		xform         = m4_translate(xform, v3(player_pos.x, player_pos.y, 0));
		draw_image_xform(player, xform, v2(150, 200), COLOR_WHITE);


		os_update();
		
		
		gfx_update();
	}

	return 0;
}