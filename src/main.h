

#define zero {0};
#define assert(cond) if(!(cond)) { on_failed_assert(__LINE__); }
#define invalid_default_case default: { assert(false); }
#define array_count(arr) (sizeof((arr)) / sizeof((arr[0])))

// #define for_enemy_simd_partial(index_name) for(int index_name = g_game.enemy_arr.index_data.min_index / 4; index_name < g_game.enemy_arr.index_data.max_index_plus_one; index_name += 4)
#define for_enemy_partial(index_name) for(int index_name = g_game.enemy_arr.index_data.min_index; index_name < g_game.enemy_arr.index_data.max_index_plus_one; index_name += 1)

// #define for_projectile_simd_partial(index_name) for(int index_name = g_game.projectile_arr.index_data.min_index / 4; index_name < g_game.projectile_arr.index_data.max_index_plus_one; index_name += 4)
#define for_projectile_partial(index_name) for(int index_name = g_game.projectile_arr.index_data.min_index; index_name < g_game.projectile_arr.index_data.max_index_plus_one; index_name += 1)
#define for_pickup_partial(index_name) for(int index_name = g_game.pickup_arr.index_data.min_index; index_name < g_game.pickup_arr.index_data.max_index_plus_one; index_name += 1)


typedef struct s_v2
{
	f32 x;
	f32 y;
} s_v2;

typedef struct s_rgb
{
	f32 r;
	f32 g;
	f32 b;
} s_rgb;


typedef enum e_texture
{
	e_texture_player,
	e_texture_enemy,
	e_texture_exp,
	e_texture_bardiche1,
	e_texture_bardiche2,
	e_texture_battle_axe1,
	e_texture_giant_club,
	e_texture_greatsword1,
	e_texture_cakez2,
	e_texture_count,
} e_texture;

global char* c_texture_path_arr[e_texture_count] = {
	"assets/crawl/player/base/vampire_m.png",
	"enemy.png",
	"assets/crawl/item/misc/misc_crystal.png",
	"assets/crawl/item/weapon/bardiche1.png",
	"assets/crawl/item/weapon/bardiche2.png",
	"assets/crawl/item/weapon/battle_axe1.png",
	"assets/crawl/item/weapon/giant_club.png",
	"assets/crawl/item/weapon/greatsword1.png",
	"assets/cakez2.png",
};
global SDL_Texture* g_texture_arr[e_texture_count];

typedef enum e_enemy_type
{
	e_enemy_type_normal,
	e_enemy_type_big,
	e_enemy_type_count,
} e_enemy_type;

typedef struct s_enemy_type
{
	e_texture texture;
	int max_health;
	s_v2 size;
} s_enemy_type;

global s_enemy_type g_enemy_type_arr[e_enemy_type_count] = {
	{e_texture_enemy, 3, {32, 32}},
	{e_texture_cakez2, 20, {128, 128}},
};

#define get_enemy_type_property(entity, name) g_enemy_type_arr[g_game.enemy_arr.type[(entity)]].name

typedef enum e_weapon
{
	e_weapon_pierce,
	e_weapon_bardiche2,
	e_weapon_battle_axe1,
	e_weapon_giant_club,
	e_weapon_count,
} e_weapon;

typedef struct s_weapon_data
{
	int delay;
	e_texture texture;
} s_weapon_data;

global s_weapon_data g_weapon_data_arr[e_weapon_count] = {
	{25, e_texture_greatsword1},
	{25, e_texture_bardiche2},
	{25, e_texture_battle_axe1},
	{25, e_texture_giant_club},
};

typedef enum e_state0
{
	e_state0_play,
} e_state0;

typedef enum e_state1
{
	e_state1_default,
	e_state1_level_up,
} e_state1;


typedef enum e_pos_area_flag
{
	e_pos_area_flag_vertical = 1 << 0,
	e_pos_area_flag_center_x = 1 << 1,
	e_pos_area_flag_center_y = 1 << 2,
} e_pos_area_flag;

typedef struct s_pos_area
{
	float spacing;
	s_v2 advance;
	s_v2 pos;
} s_pos_area;

typedef struct s_entity_iterator
{
	int index;
	int next_index;
} s_entity_iterator;

typedef struct s_entity_index_data
{
	int min_index;
	int max_index_plus_one;
} s_entity_index_data;

typedef struct s_weapon
{
	int level;
	int timer;
} s_weapon;

typedef struct s_camera
{
	s_v2 pos;
} s_camera;

typedef struct s_linear_arena
{
	u8* memory;
	int capacity;
	int used;
} s_linear_arena;

typedef struct s_dynamic_array_data
{
	int element_size;
	int capacity;
	int count;
} s_dynamic_array_data;

typedef struct s_player
{
	int weapon_count;
	int weapon_index_arr[e_weapon_count];
	s_weapon weapon_arr[e_weapon_count];
	int curr_exp;
	int level;
	s_v2 prev_pos;
	s_v2 pos;
} s_player;

typedef struct s_enemy_arr
{
	s_entity_index_data index_data;
	bool active[c_max_entities];

	s64 id[c_max_entities];
	e_enemy_type type[c_max_entities];
	int damage_taken[c_max_entities];
	int max_health[c_max_entities];
	f32 last_hit_time[c_max_entities];
	f32 speed[c_max_entities];
	s_v2 prev_pos[c_max_entities];
	s_v2 pos[c_max_entities];
	s_v2 dir[c_max_entities];
} s_enemy_arr;

typedef struct s_pickup_arr
{
	s_entity_index_data index_data;
	bool active[c_max_entities];

	bool following_player[c_max_entities];
	int exp_to_give[c_max_entities];
	f32 speed[c_max_entities];
	s_v2 prev_pos[c_max_entities];
	s_v2 pos[c_max_entities];
	s_v2 dir[c_max_entities];
} s_pickup_arr;

typedef struct s_projectile_arr
{
	s_entity_index_data index_data;
	bool active[c_max_entities];

	SDL_Texture* texture[c_max_entities];
	int pierce_count[c_max_entities];
	int ticks_left[c_max_entities];
	int already_hit_count[c_max_entities];
	s64 already_hit_arr[c_max_entities][c_max_pierces];
	s_v2 prev_pos[c_max_entities];
	s_v2 pos[c_max_entities];
	s_v2 dir[c_max_entities];
	f32 speed[c_max_entities];
} s_projectile_arr;

typedef struct s_keys
{
	bool up;
	bool down;
	bool enter;
} s_keys;

typedef struct s_entity_id
{
	int index;
	s64 id;
} s_entity_id;

typedef struct s_game
{
	s_v2 enemy_cell_min_bounds;
	s_v2 pickup_cell_min_bounds;
	s_entity_id* enemy_cell_arr[c_max_cells][c_max_cells];
	s_entity_id* pickup_cell_arr[c_max_cells][c_max_cells];
	s_camera camera;
	s64 next_entity_id;
	s_rng rng;
	s_keys keys;
	e_state0 state0;
	e_state1 state1;
	u64 level_up_seed;
	int level_up_selection;
	int level_up_triggers;
	int speed_index;
	int spawn_timer;
	s_player player;
	s_enemy_arr enemy_arr;
	s_pickup_arr pickup_arr;
	s_projectile_arr projectile_arr;
	bool key_down_arr[SDL_SCANCODE_COUNT];
} s_game;

func void update(void);
func void render(f32 interp_dt);
func f32 get_seconds32(void);
func f64 get_seconds64(void);
func s_v2 v2(f32 x, f32 y);
func s_v2 v2_1(f32 v);
func void draw_texture_topleft(s_v2 pos, s_v2 size, s_rgb color, SDL_Texture* texture);
func void v2_add_p(s_v2* out, s_v2 to_add);
func void init_game(void);
func f32 v2_length(s_v2 v);
func s_v2 v2_normalized(s_v2 v);
func void v2_add_scale_p(s_v2* out, s_v2 to_add, f32 scale);
func void v2_scale_p(s_v2* out, f32 scale);
func f32 lerp(f32 a, f32 b, f32 t);
func s_v2 lerp_v2(s_v2 a, s_v2 b, f32 t);
func SDL_Texture* load_texture(char* path);
func int make_enemy(s_v2 pos, e_enemy_type type);
func s_v2 v2_sub(s_v2 a, s_v2 b);
func s_v2 v2_from_to(s_v2 from, s_v2 to);
func s_v2 v2_from_to_normalized(s_v2 from, s_v2 to);
func void add_at_most_int(int* out, int to_add, int max_val);
func int make_projectile(s_v2 pos, s_v2 dir, SDL_Texture* texture);
func f32 v2_length_squared(s_v2 v);
func f32 v2_distance_squared(s_v2 a, s_v2 b);
func s_rgb make_color(float r, float g, float b);
func s_rgb make_color_1(float v);
func void draw_texture_center(s_v2 pos, s_v2 size, s_rgb color, SDL_Texture* texture);
func float get_game_speed(void);
func int circular_index(int index, int size);
func void circular_index_add(int* index, int to_add, int size);
func f64 at_most_f64(f64 a, f64 b);
func bool is_on_screen_center(s_v2 pos, s_v2 size);
func s_v2 v2_scale(s_v2 out, f32 scale);
func bool rect_vs_rect_topleft(s_v2 pos0, s_v2 size0, s_v2 pos1, s_v2 size1);
func bool rect_vs_rect_center(s_v2 pos0, s_v2 size0, s_v2 pos1, s_v2 size1);
func bool entities_collide(int a, int b);
func s_v2 v2_sub_scale(s_v2 a, s_v2 b, float scale);
func void remove_entity(int entity, bool* active_arr, s_entity_index_data* index_data);
func int make_exp(s_v2 pos);
func f32 v2_distance(s_v2 a, s_v2 b);
func int add_exp(s_player* player, int exp_to_give);
func s_v2 wxy(f32 x, f32 y);
func void draw_rect_topleft(s_v2 pos, s_v2 size, s_rgb color);
func int get_exp_to_level(int level);
func void on_failed_assert(int line);
func s_v2 get_center(s_v2 pos, s_v2 size);
func s_pos_area make_pos_area(s_v2 pos, s_v2 size, s_v2 element_size, float spacing, int count, int flags);
func s_v2 pos_area_get_advance(s_pos_area* area);
func void add_weapon(s_player* player, e_weapon weapon_index);
func s_v2 rand_v2(s_rng* rng);
func s_v2 rand_v2_normalized(s_rng* rng);
func f32 get_projectile_angle(int projectile_i, int num_projectiles);
func s_v2 v2_rotated(s_v2 v, float angle);
func int get_closest_enemy(s_v2 pos);
func bool is_enemy_already_hit(s64* arr, int count, int enemy);
func s_v2 get_enemy_size(int enemy);
func int ceilfi(float x);
func void draw_texture_center_camera(s_v2 pos, s_v2 size, s_rgb color, SDL_Texture* texture, s_camera camera);
func s_v2 world_to_screen(s_v2 pos, s_camera camera);
func void v2_sub_p(s_v2* out, s_v2 to_sub);
func void draw_rect_topleft_camera(s_v2 pos, s_v2 size, s_rgb color, s_camera camera);
func void get_camera_bounds(s_v2* out_min, s_v2* out_max, s_camera camera);
func int floorfi(float x);
func s_linear_arena make_linear_arena_alloc(int size);
func void* arena_get(int bytes, s_linear_arena* arena);
func s_v2 v2_add(s_v2 a, s_v2 b);
func void array_add(void** in_arr, void* new_element, s_linear_arena* arena);
func void* make_dynamic_array(int element_size, int initial_capacity, s_linear_arena* arena);
func int array_get_count(void* in_arr);
func int id_to_enemy(s_entity_id id);
func void draw_rect_center_camera(s_v2 pos, s_v2 size, s_rgb color, s_camera camera);
func int id_to_pickup(s_entity_id id);
func bool circle_vs_rect_center(s_v2 center, s_v2 in_radius, s_v2 rect_pos, s_v2 rect_size);