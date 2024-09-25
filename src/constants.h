
#define null NULL
#define func static
#define global static

typedef int64_t s64;
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

#define c_updates_per_second 60
#define c_update_delay (1.0 / c_updates_per_second)
#define c_max_entities 16384
#define c_window_width 1920
#define c_window_height 900
#define c_window_size v2(c_window_width, c_window_height)
#define c_window_center_x (c_window_width * 0.5f)
#define c_window_center_y (c_window_height * 0.5f)
#define c_window_center v2(c_window_center_x, c_window_center_y)
#define epsilon 0.000001f
#define c_pi 3.14159265359f
#define c_half_pi (c_pi * 0.5f)
#define c_tau 6.2831853071f
#define c_max_pierces 8

// @Note(tkap, 25/09/2024): Careful changing this
#define c_cell_area (c_window_width * 2)
#define c_cell_size 128.0f
#define c_max_cells 30

#define c_projectile_size v2_1(32)
#define c_player_size v2_1(48)
#define c_exp_size v2_1(24)

static const float c_game_speed_arr[] = {
	0, 0.01f, 0.1f, 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f
};