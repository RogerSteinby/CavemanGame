
#include "range.c"

// ===Game constants===

const int TILE_WIDTH = 8;
const int TILE_RADIUS_X = 40;
const int TILE_RADIUS_Y = 20;
const float TARGET_RADIUS = 16.0f;
const float PICKUP_RADIUS = 12.0f;
const int ROCK_HEALTH = 4;
const int TREE_HEALTH = 4;

// ===Conversion functions=== 

// Convert world coordinates to tile coordinates
float world_pos_to_tile_pos(float world_pos) {
	float pos = floorf(world_pos / (float)TILE_WIDTH);
	return floor(pos);
}

float tile_pos_to_world_pos(int tile_pos) {
	float pos = ((float)tile_pos * (float)TILE_WIDTH);
	return pos;
}

Vector2 round_v2_to_tile_pos(Vector2 v) {
	v.x = tile_pos_to_world_pos(world_pos_to_tile_pos(v.x));
	v.y = tile_pos_to_world_pos(world_pos_to_tile_pos(v.y));
	return v;
}

// ===Generic utilities===

// 0 - 1
float sin_breathe(float t, float rate) {
	return (sin(t * rate) + 1.0) / 2.0;
}

// Returns true if a and b are within epsilon aka almost equal to each other
bool almost_equals(float a, float b, float epsilon) {
	return fabs(a - b) <= epsilon;
}

// Animates a (value) to b (target) over time (delta_t) at a specified rate (rate)
bool animate_f32_to_target(float* value, float target, float delta_t, float rate) {
	*value += (target - *value) * (1.0 - pow(2.0f, - rate * delta_t));
	if (almost_equals(*value, target, .001f)) {
		*value = target;
		return true; // target reached
	}
	return false;
}

// The same as above, but for 2d vectors
bool animate_v2_to_target(Vector2* value, Vector2 target, float delta_t, float rate) {
	bool x_reached = animate_f32_to_target(&(value->x), target.x, delta_t, rate); 
	bool y_reached = animate_f32_to_target(&(value->y), target.y, delta_t, rate);

	return x_reached && y_reached;
}

// ===Game systems===

// Sprite system
typedef struct Sprite {
	Gfx_Image* image;
} Sprite;
// Sprite id list
typedef enum SpriteID {
	SPRITE_NIL,
	SPRITE_PLAYER,
	SPRITE_ROCK,
	SPRITE_TREE,
	SPRITE_WOOD,
	SPRITE_ITEM_ROCK,
	SPRITE_MAX,
} SpriteID;
Sprite sprites[SPRITE_MAX];
// Returns the sprite with the given id
Sprite* get_sprite(SpriteID id) {
	if (id >= 0 && id < SPRITE_MAX) {
		return &sprites[id];
	}
	return &sprites[0];
}

Vector2 get_sprite_size(Sprite* sprite) {
	return v2(sprite->image->width, sprite->image->height);
}


// Entity system
// An id list of entity types
typedef enum EntityArchetype {
	ARCH_NIL = 0,
	ARCH_ROCK = 1,
	ARCH_TREE = 2,
	ARCH_PLAYER = 3,
	ARCH_ITEM_ROCK = 4,
	ARCH_WOOD = 5,
	ARCH_MAX,
} EntityArchetype;

// Item system
typedef enum ItemID {
	ITEM_NIL,
	ITEM_ROCK,
	ITEM_WOOD,
	ITEM_MAX,
} ItemID;

typedef struct Entity {
	bool isValid;
	bool render_sprite;
	bool is_breakable;

	EntityArchetype type;
	Vector2 pos;
	int health;
	SpriteID sprite_id;
	ItemID item_id;
	bool is_item;
} Entity;

Entity items[ITEM_MAX];

typedef struct ItemData {
	int amount;
} ItemData;


Entity* get_item(ItemID id) {
	if (id >= 0 && id < ITEM_MAX) {
		return &items[id];
	}
	return &items[0];
}

#define MAX_ENTITIES 1024 // Defines the maximum number of entities allowed in the game
#define MAX_INVENTORY_ITEMS ITEM_MAX  // Defines the maximum number of items allowed in the inventory

// World. contains list of entities
typedef struct World{
	Entity entities[MAX_ENTITIES];
	ItemData inventory_items[ITEM_MAX];
} World;
World* world = 0; // Initializes the world

typedef struct WorldFrame {
	Entity* selected_entity;

} WorldFrame;

WorldFrame world_frame;

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
void destroy_entity(Entity* entity) {
	memset(entity, 0, sizeof(Entity));
}

// Test item adding
// {
// 	world->inventory_items[ITEM_ROCK].amount = 5;
// }

// Entity setup
void setup_player(Entity* en) {
	en->type = ARCH_PLAYER;
	en->sprite_id = SPRITE_PLAYER;
}
void setup_rock(Entity* en) {
	en->type = ARCH_ROCK;
	en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
	en->pos = round_v2_to_tile_pos(en->pos);
	en->pos.x += TILE_WIDTH * 0.5f;
	en->pos.y += TILE_WIDTH * 0.5f;
	en->sprite_id = SPRITE_ROCK;
	en->health = ROCK_HEALTH;
	en->is_breakable = true;
	// log("setup_rock: %f, %f", en->pos.x, en->pos.y);
}
void setup_tree(Entity* en) {
	en->type = ARCH_TREE;
	en->pos = v2(get_random_float32_in_range(-200, 200), get_random_float32_in_range(-200, 200));
	en->pos = round_v2_to_tile_pos(en->pos);
	en->pos.x += TILE_WIDTH * 0.5f;
	en->pos.y += TILE_WIDTH * 0.5f;
	en->sprite_id = SPRITE_TREE;
	en->health = TREE_HEALTH;
	en->is_breakable = true;
}

void setup_item_wood(Entity* en) {
	en->item_id = ITEM_WOOD;
	en->sprite_id = SPRITE_WOOD;
	en->is_breakable = false;
	en->is_item = true;
}

void setup_item_rock(Entity* en) {
	en->item_id = ITEM_ROCK;
	en->sprite_id = SPRITE_ITEM_ROCK;
	en->is_breakable = false;
	en->is_item = true;
}


// Convert screen coordinates to world coordinates
Vector2 mouse_pos_to_world_pos() {
	
	// get mouse position, projection, view, and window dimensions
	float mouseX = input_frame.mouse_x;
	float mouseY = input_frame.mouse_y;
	Matrix4 proj = draw_frame.projection;
	Matrix4 view = draw_frame.camera_xform;
	float windowWidth = window.width;
	float windowHeight = window.height;

	// normalize mouse coordinates
	float ndcX = (mouseX / (windowWidth * 0.5f)) - 1.0f;
	float ndcY = (mouseY / (windowHeight * 0.5f)) - 1.0f;

	// Convert and store world coordinates in a vector (Vector4)
	Vector4 worldPos = v4(ndcX, ndcY, 0, 1.0f); // convert to world coordinates
	worldPos = m4_transform(m4_inverse(proj), worldPos);
	worldPos = m4_transform(view, worldPos);

	// return as 2d vector
	return (Vector2) {worldPos.x, worldPos.y};
}


// Game entry point
int entry(int argc, char **argv) {
	
	// This is how we (optionally) configure the window.
	// To see all the settable window properties, ctrl+f "struct Os_Window" in os_interface.c

	// Set window properties
	window.title = STR("Caveman Game");
	window.width = 1280;
	window.height = 720;
	window.x = 200;
	window.y = 200;
	window.clear_color = hex_to_rgba(0x4b692fff);

	world = alloc(get_heap_allocator(), sizeof(World));	

	// world constants
	const Vector4 TILE_COL = v4(0.1, 0.1, 0.1, 0.1);
	const Vector4 SELECTED_TILE_COL = v4(0.1, 0.1, 0.1, 0.5);

	// load font
	Gfx_Font *font = load_font_from_disk(STR("C:/windows/fonts/arial.ttf"), get_heap_allocator());
	assert(font, "Failed loading arial.ttf");
	
	const u32 font_height = 48;

	// loads game images
	sprites[SPRITE_PLAYER] = (Sprite){ .image = load_image_from_disk(fixed_string("assets/player.png"), get_heap_allocator())};
	sprites[SPRITE_ROCK] = (Sprite){ .image = load_image_from_disk(fixed_string("assets/rock.png"), get_heap_allocator())};
	sprites[SPRITE_TREE] = (Sprite){ .image = load_image_from_disk(fixed_string("assets/tree.png"), get_heap_allocator())};
	sprites[SPRITE_WOOD] = (Sprite){ .image = load_image_from_disk(fixed_string("assets/wood.png"), get_heap_allocator())};
	sprites[SPRITE_ITEM_ROCK] = (Sprite){ .image = load_image_from_disk(fixed_string("assets/item_rock.png"), get_heap_allocator())};

	Entity* player_en = create_entity();
	setup_player(player_en);
	player_en->pos = v2(0, 0);
	float64 player_speed = 50.0;

	for (int i = 0; i < 10; i++) {
		Entity* en = create_entity();
		setup_rock(en);
	}

	for (int i = 0; i < 10; i++) {
		Entity* en = create_entity();
		setup_tree(en);
	}

	float64 last_time = os_get_elapsed_seconds();

	// Camera setup
	float zoom = 0.1875;
	Vector2 camera_pos;

	// Game loop
	while (!window.should_close) {
		// should close the window if escape is pressed
		reset_temporary_storage();
		world_frame = (WorldFrame){0};

		// should close the window if escape is pressed (doesnt work)
		if (is_key_just_pressed(KEY_ESCAPE)) {
			window.should_close = true;
		}
		os_update();

		// records the time passed since the last frame
		float64 now = os_get_elapsed_seconds();
		float64 delta = now - last_time;
		if ((int)now != (int)last_time) log("%.2f FPS\n%.2fms", 1.0/(now-last_time), (now-last_time)*1000);
		last_time = now;

		// Camera
		{
			Vector2 target_pos = player_en->pos;
			animate_v2_to_target(&camera_pos, target_pos, delta, 10.0f);

			draw_frame.camera_xform = m4_make_scale(v3(1.0, 1.0, 1.0));
			draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_translation(v3(camera_pos.x, camera_pos.y, 1.0)));
			draw_frame.camera_xform = m4_mul(draw_frame.camera_xform, m4_make_scale(v3(zoom, zoom, 1.0)));
		}

		Vector2 mouse_pos_world = mouse_pos_to_world_pos();
		int mouse_tile_x = world_pos_to_tile_pos(mouse_pos_world.x);
		int mouse_tile_y = world_pos_to_tile_pos(mouse_pos_world.y);

		// log("Mouse: %f, %f", mouse_tile_x, mouse_tile_y);
		// Entity selection
		{
			float smallest_dist = 0.0;

			for (int i = 0; i < MAX_ENTITIES; i++) {
				Entity* en = &world->entities[i];
				if (en->isValid && en->is_breakable) {
					Sprite* sprite = get_sprite(en->sprite_id);

					int entity_tile_x = world_pos_to_tile_pos(en->pos.x);
					int entity_tile_y = world_pos_to_tile_pos(en->pos.y);

					float dist = fabs(v2_dist(en->pos, mouse_pos_world));
				// radius
				if (dist < TARGET_RADIUS) {
					
					if (!world_frame.selected_entity || dist < smallest_dist) {
						world_frame.selected_entity = en;
						smallest_dist = dist;
					}
					
				}
		
				}
			}
		}

		// Item pickup
		{
			for (int i = 0; i < MAX_ENTITIES; i++) {
				Entity* en = &world->entities[i];
				if (en->isValid && en->is_item) {
					if (fabs(v2_dist(en->pos, player_en->pos)) < PICKUP_RADIUS) {
						 //pickup
						world->inventory_items[en->item_id].amount += 1;
						log("picked up item %d", en->item_id);
						destroy_entity(en);
					}
				}
			}
		}
		// Click to attack
		{
			Entity* selected_entity = world_frame.selected_entity;
			if (is_key_just_pressed(MOUSE_BUTTON_LEFT)) {
				consume_key_just_pressed(MOUSE_BUTTON_LEFT);
				if (selected_entity) {
					selected_entity->health -= 1;
				}
				if (selected_entity && selected_entity->health <= 0) {
					switch (selected_entity->type) {
						case ARCH_ROCK: {
							Entity* en = create_entity();
							setup_item_rock(en);
							en->pos = selected_entity->pos;
						}	break;
						case ARCH_TREE: {
							Entity* en = create_entity();
							setup_item_wood(en);
							en->pos = selected_entity->pos;
						}	break;
						default: {
							
						}	break;
						selected_entity = 0;
					}
					destroy_entity(selected_entity);
				}
			}

		}

		// tile rendering
		{
			int player_tile_x = world_pos_to_tile_pos(player_en->pos.x);
			int player_tile_y = world_pos_to_tile_pos(player_en->pos.y);

			for (int x = player_tile_x - TILE_RADIUS_X; x < player_tile_x + TILE_RADIUS_X; x++) {
				for (int y = player_tile_y - TILE_RADIUS_Y; y < player_tile_y + TILE_RADIUS_Y; y++) {
					if ((x + (y% 2 == 0)) % 2 == 0) {
						float x_pos = x * TILE_WIDTH;
						float y_pos = y * TILE_WIDTH;

						draw_rect(v2(x_pos, y_pos), v2(TILE_WIDTH, TILE_WIDTH), TILE_COL);
						}
				}
			}

			// draw_rect(v2((mouse_tile_x * TILE_WIDTH) + (TILE_WIDTH * 0.5), (mouse_tile_y * TILE_WIDTH) + (TILE_WIDTH * 0.5)), v2(TILE_WIDTH, TILE_WIDTH), COLOR_RED);
		}

		// Rendering
		{
			for (int i = 0; i < MAX_ENTITIES; i++) {
				Entity* en = &world->entities[i];
				if (en->isValid) {
					switch (en->type) {
						default:
						{	
							// xform is like the container for the entity
							Sprite* sprite = get_sprite(en->sprite_id);
							Matrix4 xform = m4_scalar(1.0);
							if (en->is_item) {
								xform         = m4_translate(xform, v3(0, 2.0 *  sin_breathe(os_get_elapsed_seconds(), 3.0), 0));
							}
							xform         = m4_translate(xform, v3(0, TILE_WIDTH * -0.5, 0)); // This moves the sprite in relation to the xform matrix
							xform         = m4_translate(xform, v3(en->pos.x, en->pos.y, 0));
							xform         = m4_translate(xform, v3(sprite->image->width * -0.5, 0, 0));

							Vector4 col = COLOR_WHITE;
							if (world_frame.selected_entity == en) {
								col = COLOR_RED;
							}
							draw_image_xform(sprite->image, xform, get_sprite_size(sprite), col);

							// draw_text(font, sprint(get_temporary_allocator(), STR("%.2f, %.2f"), en->pos.x, en->pos.y), font_height, en->pos, v2(0.1, 0.1), COLOR_WHITE); // debug
						}
					}
				}
			}
}
		// player movement
		{
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
			player_en->pos = v2_add(player_en->pos, v2_mulf(input_axis, player_speed * delta));
		}
		
		gfx_update();
	}

	return 0;
}