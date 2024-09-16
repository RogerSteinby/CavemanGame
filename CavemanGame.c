
typedef enum EntityArchetype {
	arch_nil = 0,
	arch_rock = 1,
	arch_tree = 2,
	arch_player = 3,
} EntityArchetype;

// Sprite system
typedef struct Sprite {
	Gfx_Image* image;
	Vector2 size;
} Sprite;
typedef enum SpriteID {
	SPRITE_nil,
	SPRITE_PLAYER,
	SPRITE_ROCK,
	SPRITE_TREE,
	SPRITE_MAX,
} SpriteID;
Sprite sprites[SPRITE_MAX];
Sprite* get_sprite(SpriteID id) {
	if (id >= 0 && id < SPRITE_MAX) {
		return &sprites[id];
	}
	return &sprites[0];
}

typedef struct Entity {
	bool isValid;
	bool render_sprite;

	EntityArchetype type;
	Vector2 pos;

	SpriteID sprite_id;
} Entity;
#define MAX_ENTITIES 1024

typedef struct World{
	Entity entities[MAX_ENTITIES];
} World;
World* world = 0;

Entity* create_entity() {
	Entity* entity_found = 0;
	for (int i = 0; i <= MAX_ENTITIES; i++) {
		Entity* existing_entity = &world->entities[i];
		if (!existing_entity->isValid) {
			entity_found = existing_entity;
			break;
		}
	}
	assert(entity_found, "Failed to create entity");
	entity_found->isValid = true;
	return entity_found;
}

// Entity destruction
void destroy_entity() {
	memset(entry, 0, sizeof(*entry));
}

// Entity setup
void setup_player(Entity* en) {
	en->type = arch_player;
	en->sprite_id = SPRITE_PLAYER;
}
void setup_rock(Entity* en) {
	en->type = arch_rock;
	en->pos = v2(rand() % window.width, rand() % window.height);
	en->sprite_id = SPRITE_ROCK;
}
void setup_tree(Entity* en) {
	en->type = arch_tree;
	en->pos = v2(rand() % window.width, rand() % window.height);
	en->sprite_id = SPRITE_TREE;
}

int entry(int argc, char **argv) {
	
	// This is how we (optionally) configure the window.
	// To see all the settable window properties, ctrl+f "struct Os_Window" in os_interface.c
	window.title = STR("Caveman Game");
	window.width = 1280;
	window.height = 720;
	window.x = 200;
	window.y = 200;
	window.clear_color = hex_to_rgba(0x4b692fff);

	world = alloc(get_heap_allocator(), sizeof(World));	

	// loads game images
	sprites[SPRITE_PLAYER] = (Sprite){ .image = load_image_from_disk(fixed_string("assets/player.png"), get_heap_allocator()), .size = v2(6, 12) };
	sprites[SPRITE_ROCK] = (Sprite){ .image = load_image_from_disk(fixed_string("assets/rock.png"), get_heap_allocator()), .size = v2(6, 4) };
	sprites[SPRITE_TREE] = (Sprite){ .image = load_image_from_disk(fixed_string("assets/tree.png"), get_heap_allocator()), .size = v2(34, 44) };

	Entity* player_en = create_entity();
	setup_player(player_en);
	player_en->pos = v2(0, 0);

	for (int i = 0; i < 10; i++) {
		Entity* en = create_entity();
		setup_rock(en);
		en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
	}

	for (int i = 0; i < 10; i++) {
		Entity* en = create_entity();
		setup_tree(en);
		en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
	}

	float64 player_speed = .01;

	float64 last_time = os_get_elapsed_seconds();

	// game loop
	while (!window.should_close) {
		// should close the window if escape is pressed
		if (is_key_just_pressed(KEY_ESCAPE)) {
			window.should_close = true;
		}

		float zoom = 0.1875;
		draw_frame.view = m4_make_scale(v3(zoom, zoom, 1.0));

		for (int i = 0; i < MAX_ENTITIES; i++) {
			Entity* en = &world->entities[i];
			if (en->isValid) {
				switch (en->type) {
					default:
					{
						Sprite* sprite = get_sprite(en->sprite_id);
						Matrix4 xform = m4_scalar(1.0);
						xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
						xform         = m4_translate(xform, v3(sprite->size.x * .5, 0, 0));
						draw_image_xform(sprite->image, xform, sprite->size, COLOR_WHITE);
					}
				}
			}
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

		// player_en->pos = player_en->pos + (input_axis * 10.0);
		player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, player_speed));

		reset_temporary_storage();
		
		// records the time passed since the last frame
		float64 now = os_get_elapsed_seconds();
		float64 delta = now - last_time;
		if ((int)now != (int)last_time) log("%.2f FPS\n%.2fms", 1.0/(now-last_time), (now-last_time)*1000);
		last_time = now;

		os_update();
		
		
		gfx_update();
	}

	return 0;
}