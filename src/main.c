
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <intrin.h>

#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(x)
#include "include/stb_image.h"
#pragma warning(pop)

#include "SDL3/SDL.h"
#include "constants.h"
#include "rng.h"
#include "cells.h"
#include "main.h"

global SDL_Renderer* g_renderer = null;
global f32 g_delta = 0;
global f32 g_multiplied_delta = 0;
global u64 g_start_cycles = 0;
global u64 g_cycle_frequency = 0;
global s_game g_game = zero;
global f32 multiplied_render_time;
global s_linear_arena g_arena;
global u64 g_time_arr[32];
global int g_time_count;

#include "rng.c"
#include "cells.c"

int main(void)
{
	SDL_Window* window = null;
	SDL_Event event = zero;

	SDL_Init(SDL_INIT_VIDEO);
	bool success = SDL_CreateWindowAndRenderer("coffee_lava", c_window_width, c_window_height, 0, &window, &g_renderer);
	assert(success);

	g_game.rng = make_rng(__rdtsc());

	g_arena = make_linear_arena_alloc(100 * 1024 * 1024);

	SDL_SetRenderVSync(g_renderer, 1);

	// stbi_set_flip_vertically_on_load(true);

	for(int i = 0; i < e_texture_count; i += 1) {
		g_texture_arr[i] = load_texture(c_texture_path_arr[i]);
	}

	g_cycle_frequency = SDL_GetPerformanceFrequency();
	g_start_cycles = SDL_GetPerformanceCounter();
	SDL_Time time_before = g_start_cycles;
	f64 update_timer = 0;

	init_game();

	while(true) {
		u64 now = SDL_GetPerformanceCounter();
		u64 passed = now - time_before;
		f64 delta64 = (passed / (f64)g_cycle_frequency);
		g_delta = (f32)delta64;
		g_multiplied_delta = g_delta * get_game_speed();
		time_before = now;

		g_arena.used = 0;
		g_game.keys = (s_keys)zero;
		while(SDL_PollEvent(&event)) {
			if(event.type == SDL_EVENT_QUIT) {
				exit(0);
			}

			if(event.type == SDL_EVENT_KEY_DOWN) {

				if(event.key.key == SDLK_UP) {
					g_game.keys.up = true;
				}
				if(event.key.key == SDLK_DOWN) {
					g_game.keys.down = true;
				}
				if(event.key.key == SDLK_RETURN) {
					g_game.keys.enter = true;
				}

				if(event.key.key == SDLK_KP_PLUS) {
					circular_index_add(&g_game.speed_index, 1, array_count(c_game_speed_arr));
					printf("Speed: %f\n", get_game_speed());
				}
				if(event.key.key == SDLK_KP_MINUS) {
					circular_index_add(&g_game.speed_index, -1, array_count(c_game_speed_arr));
					printf("Speed: %f\n", get_game_speed());
				}
				if(event.key.key == SDLK_RIGHT && g_game.speed_index == 0) {
					update_timer += c_update_delay;
				}
			}
		}

		{
			const bool* key_state_arr = null;
			int num_keys = 0;
			key_state_arr = SDL_GetKeyboardState(&num_keys);

			for(int i = 0; i < num_keys; i += 1) {
				g_game.key_down_arr[i] = g_game.key_down_arr[i] || key_state_arr[i];
			}
		}

		bool updated_once = false;
		update_timer += delta64 * get_game_speed();
		f64 temp_timer = at_most_f64(0.1, update_timer);
		while(temp_timer >= c_update_delay) {
			temp_timer -= c_update_delay;
			update_timer -= c_update_delay;
			update();
			updated_once = true;
		}
		if(updated_once) {
			memset(g_game.key_down_arr, 0, sizeof(g_game.key_down_arr));
		}
		f32 interp_dt = (f32)(temp_timer / c_update_delay);
		render(interp_dt);
	}

	return 0;
}

func void init_game(void)
{
	g_game.player.pos.x = c_window_center_x;
	g_game.player.pos.y = c_window_center_y;
	g_game.player.prev_pos = g_game.player.pos;
	g_game.player.level = 1;
	g_game.speed_index = 5;

	add_weapon(&g_game.player, e_weapon_giant_club);
}

func void update(void)
{
	g_time_count = 0;

	s_enemy_arr* enemy_arr = &g_game.enemy_arr;
	s_projectile_arr* projectile_arr = &g_game.projectile_arr;
	s_inactive_pickup_arr* inactive_pickup_arr = &g_game.inactive_pickup_arr;
	s_active_pickup_arr* active_pickup_arr = &g_game.active_pickup_arr;
	s_aoe_arr* aoe_arr = &g_game.aoe_arr;

	if(g_game.state1 == e_state1_default && g_game.level_up_triggers > 0) {
		g_game.level_up_triggers -= 1;
		g_game.state1 = e_state1_level_up;
		g_game.level_up_seed = __rdtsc();
		g_game.level_up_selection = 0;
		return;
	}

	g_game.spawn_timer += 1;
	if(g_game.spawn_timer >= 3) {
		g_game.spawn_timer = 0;
		f32 angle = randf32(&g_game.rng) * c_tau;
		s_v2 pos = v2(
			cosf(angle) * c_window_width * 0.6f,
			sinf(angle) * c_window_width * 0.6f
		);
		pos.x += g_game.player.pos.x;
		pos.y += g_game.player.pos.y;

		if(rand_chance100(&g_game.rng, 1)) {
			make_enemy(pos, e_enemy_type_big);
		}
		else {
			make_enemy(pos, e_enemy_type_normal);
		}
	}

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		build enemy cell spatial partition start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		for(int y = 0; y < c_max_cells; y += 1) {
			for(int x = 0; x < c_max_cells; x += 1) {
				g_game.enemy_cell_arr[y][x] = make_dynamic_array(sizeof(s_entity_id), 4, &g_arena);
			}
		}
		s_v2 center = g_game.player.pos;
		float half_area_size = c_cell_area * 0.5f;
		s_v2 min_bounds = v2_sub(center, v2_1(half_area_size));
		g_game.enemy_cell_min_bounds = min_bounds;
		for_enemy_partial(i) {
			if(!enemy_arr->active[i]) { continue; }
			assert(get_enemy_type_property(i, size).x < c_cell_size * 2);
			assert(get_enemy_type_property(i, size).y < c_cell_size * 2);
			s_v2 pos = enemy_arr->pos[i];
			int x_index = floorfi((pos.x - min_bounds.x) / c_cell_size);
			int y_index = floorfi((pos.y - min_bounds.y) / c_cell_size);
			if(x_index < 0 || x_index >= c_max_cells) { continue; }
			if(y_index < 0 || y_index >= c_max_cells) { continue; }
			s_entity_id id = zero;
			id.index = i;
			id.id = enemy_arr->id[i];
			array_add(&g_game.enemy_cell_arr[y_index][x_index], &id, &g_arena);
		}
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		build enemy cell spatial partition end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		build inactive pickup cell spatial partition start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		for(int y = 0; y < c_max_cells; y += 1) {
			for(int x = 0; x < c_max_cells; x += 1) {
				g_game.inactive_pickup_cell_arr[y][x] = make_dynamic_array(sizeof(s_entity_id), 4, &g_arena);
			}
		}
		s_v2 center = g_game.player.pos;
		float half_area_size = c_cell_area * 0.5f;
		s_v2 min_bounds = v2_sub(center, v2_1(half_area_size));
		g_game.inactive_pickup_cell_min_bounds = min_bounds;
		for_inactive_pickup_partial(i) {
			if(!inactive_pickup_arr->active[i]) { continue; }
			s_v2 pos = inactive_pickup_arr->pos[i];
			int x_index = floorfi((pos.x - min_bounds.x) / c_cell_size);
			int y_index = floorfi((pos.y - min_bounds.y) / c_cell_size);
			if(x_index < 0 || x_index >= c_max_cells) { continue; }
			if(y_index < 0 || y_index >= c_max_cells) { continue; }
			s_entity_id id = zero;
			id.index = i;
			array_add(&g_game.inactive_pickup_cell_arr[y_index][x_index], &id, &g_arena);
		}
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		build inactive pickup cell spatial partition end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		build active pickup cell spatial partition start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		for(int y = 0; y < c_max_cells; y += 1) {
			for(int x = 0; x < c_max_cells; x += 1) {
				g_game.active_pickup_cell_arr[y][x] = make_dynamic_array(sizeof(s_entity_id), 4, &g_arena);
			}
		}
		s_v2 center = g_game.player.pos;
		float half_area_size = c_cell_area * 0.5f;
		s_v2 min_bounds = v2_sub(center, v2_1(half_area_size));
		g_game.active_pickup_cell_min_bounds = min_bounds;
		for_active_pickup_partial(i) {
			if(!active_pickup_arr->active[i]) { continue; }
			s_v2 pos = active_pickup_arr->pos[i];
			int x_index = floorfi((pos.x - min_bounds.x) / c_cell_size);
			int y_index = floorfi((pos.y - min_bounds.y) / c_cell_size);
			if(x_index < 0 || x_index >= c_max_cells) { continue; }
			if(y_index < 0 || y_index >= c_max_cells) { continue; }
			s_entity_id id = zero;
			id.index = i;
			array_add(&g_game.active_pickup_cell_arr[y_index][x_index], &id, &g_arena);
		}
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		build active pickup cell spatial partition end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	memcpy(enemy_arr->prev_pos, enemy_arr->pos, sizeof(enemy_arr->prev_pos));
	memcpy(projectile_arr->prev_pos, projectile_arr->pos, sizeof(projectile_arr->prev_pos));
	memcpy(aoe_arr->prev_pos, aoe_arr->pos, sizeof(aoe_arr->prev_pos));
	memcpy(active_pickup_arr->prev_pos, active_pickup_arr->pos, sizeof(active_pickup_arr->prev_pos));

	g_game.player.prev_pos = g_game.player.pos;

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		player movement start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		s_player* player = &g_game.player;
		s_v2 movement = zero;
		if(g_game.key_down_arr[SDL_SCANCODE_A]) {
			movement.x -= 1;
		}
		if(g_game.key_down_arr[SDL_SCANCODE_D]) {
			movement.x += 1;
		}
		if(g_game.key_down_arr[SDL_SCANCODE_W]) {
			movement.y -= 1;
		}
		if(g_game.key_down_arr[SDL_SCANCODE_S]) {
			movement.y += 1;
		}
		movement = v2_normalized(movement);
		v2_add_scale_p(&player->pos, movement, 4);
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		player movement end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		move falling aoe start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	for_aoe_partial(i) {
		if(!aoe_arr->active[i]) { continue; }
		if(aoe_arr->falling[i]) {
			f32 distance = 0;
			aoe_arr->pos[i] = move_towards_v2(aoe_arr->pos[i], aoe_arr->target_pos[i], aoe_arr->speed[i], &distance);
			if(distance < 1) {
				aoe_arr->falling[i] = false;
			}
		}
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		move falling aoe end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		aoe damage enemies and expire start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	for_aoe_partial(i) {
		if(!aoe_arr->active[i]) { continue; }
		if(aoe_arr->falling[i]) { continue; }

		aoe_arr->hit_timer[i] -= 1;
		if(aoe_arr->hit_timer[i] <= 0) {
			aoe_arr->hit_timer[i] = 60;

			s_cell_iterator it = zero;
			while(get_enemy_in_cells(&it, aoe_arr->pos[i], aoe_arr->size[i])) {
				assert(enemy_arr->active[it.index]);
				damage_enemy(it.index, 1);
			}
		}
		aoe_arr->ticks_left[i] -= 1;
		if(aoe_arr->ticks_left[i] <= 0) {
			remove_entity(i, aoe_arr->active, &aoe_arr->index_data);
		}
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		aoe damage enemies and expire end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		active pickup move start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	for_active_pickup_partial(i) {
		if(!active_pickup_arr->active[i]) { continue; }
		s_v2 dir = v2_from_to_normalized(active_pickup_arr->pos[i], g_game.player.pos);
		v2_add_scale_p(&active_pickup_arr->pos[i], dir, active_pickup_arr->speed[i]);
		active_pickup_arr->speed[i] += 0.1f;
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		active pickup move end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		pickup start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		u64 before = SDL_GetPerformanceCounter();

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		trigger start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			s_cell_iterator it = zero;
			while(get_inactive_pickup_in_cells(&it, g_game.player.pos, c_pickup_trigger_radius)) {
				assert(inactive_pickup_arr->active[it.index]);
				s_v2 dir = v2_from_to_normalized(g_game.player.pos, inactive_pickup_arr->pos[it.index]);
				v2_add_scale_p(&inactive_pickup_arr->pos[it.index], dir, 64);

				activate_pickup_and_remove(it.index);
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		trigger end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		collide with player start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			s_cell_iterator it = zero;
			while(get_active_pickup_in_cells(&it, g_game.player.pos, c_player_size)) {
				assert(active_pickup_arr->active[it.index]);

				switch(active_pickup_arr->type[it.index]) {
					case e_pickup_exp0: {
						add_exp(&g_game.player, active_pickup_arr->exp_to_give[it.index]);
					} break;

					case e_pickup_vacuum_exp: {
						for_inactive_pickup_partial(i) {
							if(!inactive_pickup_arr->active[i]) { continue; }
							activate_pickup_and_remove(i);
						}
					} break;

					invalid_default_case;
				}
				remove_entity(it.index, active_pickup_arr->active, &active_pickup_arr->index_data);
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		collide with player end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		u64 passed = SDL_GetPerformanceCounter() - before;
		g_time_arr[g_time_count] = passed;
		g_time_count += 1;
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		pickup end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		enemy movement start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	for_enemy_partial(i) {
		if(!enemy_arr->active[i]) { continue; }
		s_v2 dir = v2_from_to_normalized(enemy_arr->pos[i], g_game.player.pos);
		v2_add_scale_p(&enemy_arr->pos[i], dir, enemy_arr->speed[i]);
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		enemy movement end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		projectile collision start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		u64 before = SDL_GetPerformanceCounter();
		for_projectile_partial(i) {
			if(!projectile_arr->active[i]) { continue; }
			assert(projectile_arr->pierce_count[i] <= c_max_pierces);

			s_v2 projectile_pos = projectile_arr->pos[i];
			s_cell_iterator it = zero;
			while(get_enemy_in_cells(&it, projectile_pos, projectile_arr->size[i])) {
				assert(enemy_arr->active[it.index]);
				if(is_enemy_already_hit(projectile_arr->already_hit_arr[i], projectile_arr->already_hit_count[i], it.index)) { continue; }

				s_v2 push_dir = v2_scale(projectile_arr->dir[i], 5.0f);
				v2_add_p(&enemy_arr->pos[it.index], push_dir);
				damage_enemy(it.index, 1);
				if(projectile_arr->pierce_count[i] <= 0) {
					remove_entity(i, projectile_arr->active, &projectile_arr->index_data);
					break;
				}
				projectile_arr->pierce_count[i] -= 1;
			}
		}
		u64 passed = SDL_GetPerformanceCounter() - before;
		g_time_arr[g_time_count] = passed;
		g_time_count += 1;
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		projectile collision end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		projectile movement start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	for_projectile_partial(i) {
		if(!projectile_arr->active[i]) { continue; }
		projectile_arr->dir[i].y += projectile_arr->gravity[i];
		v2_add_scale_p(&projectile_arr->pos[i], projectile_arr->dir[i], projectile_arr->speed[i]);
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		projectile movement end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		player shoot start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		s_player* player = &g_game.player;
		for(int i = 0; i < player->weapon_count; i += 1) {
			e_weapon weapon_type = player->weapon_index_arr[i];
			s_weapon* weapon = &player->weapon_arr[weapon_type];
			int delay = get_weapon_delay(*weapon, weapon_type);
			add_at_most_int(&weapon->timer, 1, delay);
			if(weapon->timer >= delay) {
				weapon->timer -= delay;
				switch(weapon_type) {

					case e_weapon_pierce: {
						int closest = get_closest_enemy(player->pos);
						s_v2 base_dir;
						if(closest >= 0) {
							base_dir = v2_from_to_normalized(player->pos, enemy_arr->pos[closest]);
						}
						else {
							base_dir = rand_v2_normalized(&g_game.rng);
						}
						int projectile = make_projectile(player->pos, v2_1(32), base_dir, g_texture_arr[g_weapon_data_arr[weapon_type].texture]);
						// nocheckin
						// projectile_arr->pierce_count[projectile] = weapon->level + 1;
						projectile_arr->pierce_count[projectile] = 1;
					} break;

					case e_weapon_giant_club:
					{
						int closest = get_closest_enemy(player->pos);
						s_v2 base_dir;
						if(closest >= 0) {
							base_dir = v2_from_to_normalized(player->pos, enemy_arr->pos[closest]);
						}
						else {
							base_dir = rand_v2_normalized(&g_game.rng);
						}
						int num_projectiles = weapon->level;
						for(int projectile_i = 0; projectile_i < num_projectiles; projectile_i += 1) {
							float angle = get_projectile_angle(projectile_i, num_projectiles);
							s_v2 dir = v2_rotated(base_dir, angle);
							make_projectile(player->pos, v2_1(32), dir, g_texture_arr[g_weapon_data_arr[weapon_type].texture]);
						}
					} break;

					case e_weapon_battle_axe1: {
						int num_projectiles = weapon->level;
						for(int projectile_i = 0; projectile_i < num_projectiles; projectile_i += 1) {
							float angle = randf_range(&g_game.rng, -c_pi * 0.25f, c_pi * 0.25f) - c_pi * 0.5f;
							s_v2 dir = v2_from_angle(angle);
							v2_scale_p(&dir, 2);
							int projectile = make_projectile(g_game.player.pos, v2_1(64), dir, g_texture_arr[g_weapon_data_arr[weapon_type].texture]);
							projectile_arr->gravity[projectile] = 0.1f;
						}
					} break;

					case e_weapon_bardiche2: {
						int num_projectiles = weapon->level;
						s_v2 c_size_arr[] = {wxy(0.25f, 0.25f), wxy(0.5f, 0.5f), wxy(0.75f, 0.75f)};
						for(int projectile_i = 0; projectile_i < num_projectiles; projectile_i += 1) {
							int enemy = -1;

							int possible_target_arr[512];
							int possible_target_count = 0;
							for(int attempt_i = 0; attempt_i < array_count(c_size_arr); attempt_i += 1) {
								s_cell_iterator it = zero;
								while(get_enemy_in_cells(&it, g_game.player.pos, c_size_arr[attempt_i])) {
									possible_target_arr[possible_target_count] = it.index;
									possible_target_count += 1;
									if(possible_target_count >= 512) { break; }
								}
								if(possible_target_count > 0) {
									int rand_index = rand_range_ie(&g_game.rng, 0, possible_target_count);
									enemy = possible_target_arr[rand_index];
									break;
								}
							}

							s_v2 aoe_target_pos;
							if(enemy == -1) {
								float x_angle = randf32(&g_game.rng) * c_tau;
								float y_angle = randf32(&g_game.rng) * c_tau;
								float x = cosf(x_angle);
								float y = sinf(y_angle);
								aoe_target_pos = v2_scale(v2(x, y), c_window_width * 0.2f);
								v2_add_p(&aoe_target_pos, g_game.player.pos);
							}
							else {
								aoe_target_pos = enemy_arr->pos[enemy];
							}

							float angle = randf_range(&g_game.rng, -c_pi * 0.25f, c_pi * 0.25f) - c_pi * 0.5f;
							s_v2 aoe_pos = v2_scale(v2_from_angle(angle), 500);
							v2_add_p(&aoe_pos, aoe_target_pos);

							int aoe = make_aoe(aoe_pos, v2_1(64), g_texture_arr[g_weapon_data_arr[weapon_type].texture]);
							aoe_arr->ticks_left[aoe] = 200;
							aoe_arr->falling[aoe] = true;
							aoe_arr->target_pos[aoe] = aoe_target_pos;
							aoe_arr->speed[aoe] = 20;
						}
					} break;

					case e_weapon_lightning_bolt: {
						s_cell_iterator it = zero;
						int possible_target_arr[1024];
						int possible_target_count = 0;
						while(get_enemy_in_cells(&it, g_game.player.pos, wxy(0.9f, 0.9f))) {
							possible_target_arr[possible_target_count] = it.index;
							possible_target_count += 1;
							if(possible_target_count >= 1024) { break; }
						}
						if(possible_target_count > 0) {
							int rand_index = rand_range_ie(&g_game.rng, 0, possible_target_count);
							int enemy = possible_target_arr[rand_index];
							damage_enemy(enemy, 1);
							make_visual_effect(enemy_arr->pos[enemy], e_visual_effect_lightning_bolt);
						}
					} break;

					invalid_default_case;
				}
			}
		}
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		player shoot end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		projectile expire start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	for_projectile_partial(i) {
		if(!projectile_arr->active[i]) { continue; }
		projectile_arr->ticks_left[i] -= 1;
		if(projectile_arr->ticks_left[i] <= 0) {
			remove_entity(i, projectile_arr->active, &projectile_arr->index_data);
		}
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		projectile expire end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

}

func void render(f32 interp_dt)
{
	SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 0);
	SDL_RenderClear(g_renderer);

	multiplied_render_time += g_delta * get_game_speed();

	s_enemy_arr* enemy_arr = &g_game.enemy_arr;
	s_projectile_arr* projectile_arr = &g_game.projectile_arr;
	s_inactive_pickup_arr* inactive_pickup_arr = &g_game.inactive_pickup_arr;
	s_active_pickup_arr* active_pickup_arr = &g_game.active_pickup_arr;
	s_aoe_arr* aoe_arr = &g_game.aoe_arr;
	s_visual_effect_arr* visual_effect_arr = &g_game.visual_effect_arr;

	{
		s_player player = g_game.player;
		s_v2 pos = lerp_v2(player.prev_pos, player.pos, interp_dt);
		v2_sub_p(&pos, c_window_center);
		g_game.camera.pos = pos;
	}

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		background start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		float size = 128;
		s_v2 min_bounds;
		s_v2 max_bounds;
		get_camera_bounds(&min_bounds, &max_bounds, g_game.camera);
		int start_x = floorfi(min_bounds.x / size);
		int start_y = floorfi(min_bounds.y / size);
		int tiles_right = ceilfi(max_bounds.x / size);
		int tiles_down = ceilfi(max_bounds.y / size);
		for(int y = start_y; y < tiles_down; y += 1) {
			for(int x = start_x; x < tiles_right; x += 1) {
				s_v2 pos = v2_scale(v2((f32)x, (f32)y), size);
				s_rgb color;
				if((x + y) & 1) {
					color = make_color(0, 0.2f, 0);
				}
				else {
					color = make_color(0, 0.21f, 0);
				}
				draw_rect_topleft_camera(pos, v2_1(size), color, g_game.camera);
			}
		}
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		background end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


	for_aoe_partial(i) {
		if(!aoe_arr->active[i]) { continue; }
		s_v2 pos = lerp_v2(aoe_arr->prev_pos[i], aoe_arr->pos[i], interp_dt);
		draw_texture_center_camera(pos, aoe_arr->size[i], make_color_1(1), aoe_arr->texture[i], g_game.camera);
	}

	for_enemy_partial(i) {
		if(!enemy_arr->active[i]) { continue; }
		s_v2 pos = lerp_v2(enemy_arr->prev_pos[i], enemy_arr->pos[i], interp_dt);
		f32 flash = multiplied_render_time - enemy_arr->last_hit_time[i];
		flash = ease_linear_advanced(flash, 0, 0.25f, 10, 1);
		draw_texture_center_camera(pos, get_enemy_size(i), make_color_1(flash), g_texture_arr[get_enemy_type_property(i, texture)], g_game.camera);
	}

	for_inactive_pickup_partial(i) {
		if(!inactive_pickup_arr->active[i]) { continue; }
		s_v2 pos = inactive_pickup_arr->pos[i];
		e_pickup type = inactive_pickup_arr->type[i];
		e_texture texture = g_pickup_data_arr[type].texture;
		draw_texture_center_camera(pos, c_exp_size, make_color_1(1), g_texture_arr[texture], g_game.camera);
	}

	for_active_pickup_partial(i) {
		if(!active_pickup_arr->active[i]) { continue; }
		s_v2 pos = lerp_v2(active_pickup_arr->prev_pos[i], active_pickup_arr->pos[i], interp_dt);
		e_pickup type = active_pickup_arr->type[i];
		e_texture texture = g_pickup_data_arr[type].texture;
		draw_texture_center_camera(pos, c_exp_size, make_color_1(1), g_texture_arr[texture], g_game.camera);
	}

	for_projectile_partial(i) {
		if(!projectile_arr->active[i]) { continue; }
		s_v2 pos = lerp_v2(projectile_arr->prev_pos[i], projectile_arr->pos[i], interp_dt);
		draw_texture_center_camera(pos, projectile_arr->size[i], make_color_1(1), projectile_arr->texture[i], g_game.camera);
	}

	{
		s_player player = g_game.player;
		s_v2 pos = lerp_v2(player.prev_pos, player.pos, interp_dt);
		draw_texture_center_camera(pos, c_player_size, make_color_1(1), g_texture_arr[e_texture_player], g_game.camera);
	}

	for_visual_effect_partial(i) {
		if(!visual_effect_arr->active[i]) { continue; }
		visual_effect_arr->time_passed[i] += g_multiplied_delta;
		bool expired = false;
		switch(visual_effect_arr->type[i]) {
			case e_visual_effect_lightning_bolt: {
				s_v2 pos = visual_effect_arr->pos[i];
				f32 y_screen = world_to_screen(pos, g_game.camera).y;
				s_v2 top = v2_sub(pos, v2(0, y_screen));
				s_v2 size = v2_1(32);
				int how_many_fit = ceilfi((pos.y - top.y) / (size.y * 0.5f));

				for(int fit_i = 0; fit_i < how_many_fit; fit_i += 1) {
					float p = index_count_safe_div(fit_i, how_many_fit);
					s_v2 temp_pos = lerp_v2(top, pos, p);
					draw_texture_center_camera(temp_pos, size, make_color_1(1), g_texture_arr[e_texture_lightning_bolt], g_game.camera);
				}

				if(visual_effect_arr->time_passed[i] >= 0.15f) {
					expired = true;
				}

			} break;

			invalid_default_case;
		}
		if(expired) {
			remove_entity(i, visual_effect_arr->active, &visual_effect_arr->index_data);
		}
	}


	switch(g_game.state0) {
		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		state0 play start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		case e_state0_play: {

			switch(g_game.state1) {
				case e_state1_default: {
					draw_rect_topleft(wxy(0.2f, 0.02f), wxy(0.6f, 0.04f), make_color_1(0.2f));
					f32 exp_percent = g_game.player.curr_exp / (f32)get_exp_to_level(g_game.player.level);
					draw_rect_topleft(wxy(0.2f, 0.02f), wxy(0.6f * exp_percent, 0.04f), make_color(0.2f, 0.2f, 0.9f));

				} break;

				case e_state1_level_up: {

					if(g_game.keys.up) {
						circular_index_add(&g_game.level_up_selection, -1, 3);
					}
					if(g_game.keys.down) {
						circular_index_add(&g_game.level_up_selection, 1, 3);
					}

					s_v2 panel_pos = wxy(0.4f, 0.2f);
					s_v2 panel_size = wxy(0.2f, 0.6f);
					draw_rect_topleft(panel_pos, panel_size, make_color_1(0.2f));

					s_rng rng = make_rng(g_game.level_up_seed);
					int option_count = 0;
					e_weapon option_arr[e_weapon_count];
					for(int i = 0; i < e_weapon_count; i += 1) {
						option_arr[option_count] = i;
						option_count += 1;
					}
					e_weapon choice_arr[3];
					for(int i = 0; i < 3; i += 1) {
						int index = rand_range_ie(&rng, 0, option_count);
						e_weapon weapon_type = option_arr[index];
						choice_arr[i] = weapon_type;
						option_count -= 1;
						option_arr[index] = option_arr[option_count];
					}
					s_v2 element_size = v2_1(64);
					s_pos_area area = make_pos_area(
						panel_pos, panel_size, element_size, 32, 3, e_pos_area_flag_vertical | e_pos_area_flag_center_x | e_pos_area_flag_center_y
					);
					e_weapon weapon_unlocked = -1;
					for(int i = 0; i < 3; i += 1) {
						e_weapon weapon_type = choice_arr[i];

						s_rgb color = make_color_1(0.5f);
						bool selected = i == g_game.level_up_selection;
						if(selected) {
							color = make_color_1(1);
						}
						draw_texture_topleft(pos_area_get_advance(&area), element_size, color, g_texture_arr[g_weapon_data_arr[weapon_type].texture]);

						if(selected && weapon_unlocked == -1 && g_game.keys.enter) {
							weapon_unlocked = weapon_type;
						}
					}
					if(weapon_unlocked >= 0) {
						bool already_has_weapon = false;
						for(int i = 0; i < g_game.player.weapon_count; i += 1) {
							if(g_game.player.weapon_index_arr[i] == weapon_unlocked) {
								g_game.player.weapon_arr[weapon_unlocked].level += 1;
								already_has_weapon = true;
								break;
							}
						}
						if(!already_has_weapon) {
							add_weapon(&g_game.player, weapon_unlocked);
						}
						g_game.state1 = e_state1_default;
					}
				} break;

				invalid_default_case;
			}
		} break;
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		state0 play end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		invalid_default_case;
	}

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		weapon ui start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		s_v2 element_size = v2_1(48);
		s_pos_area area = make_pos_area(v2(4, 4), v2(200, 48), element_size, 4, -1, 0);
		for(int weapon_i = 0; weapon_i < g_game.player.weapon_count; weapon_i += 1) {
			e_weapon weapon_type = g_game.player.weapon_index_arr[weapon_i];
			draw_texture_topleft(pos_area_get_advance(&area), element_size, make_color_1(1), g_texture_arr[g_weapon_data_arr[weapon_type].texture]);
		}
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		weapon ui end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	// @Note(tkap, 26/09/2024): "profiler"
	// {
	// 	s_v2 pos = v2(0, 100);
	// 	for(int i = 0; i < g_time_count; i += 1) {
	// 		u64 t = g_time_arr[i];
	// 		f64 seconds = t / (f64)g_cycle_frequency;
	// 		f64 us = seconds * 1000000;
	// 		f32 width = (f32)(us / 100.0 * c_window_width);
	// 		s_v2 size = v2(width, 24.0f);
	// 		draw_rect_topleft(pos, size, make_color_1((i & 1) ? 0.5f : 0.25f));
	// 		pos.y += 28;
	// 	}
	// }

	SDL_RenderPresent(g_renderer);
}

func f32 get_seconds32(void)
{
	u64 now = SDL_GetPerformanceCounter();
	u64 passed = g_start_cycles - now;
	return (f32)(passed / (f64)g_cycle_frequency);
}

func f64 get_seconds64(void)
{
	u64 now = SDL_GetPerformanceCounter();
	u64 passed = g_start_cycles - now;
	return (passed / (f64)g_cycle_frequency);
}

func void draw_texture_center(s_v2 pos, s_v2 size, s_rgb color, SDL_Texture* texture)
{
	pos.x -= size.x * 0.5f;
	pos.y -= size.y * 0.5f;
	draw_texture_topleft(pos, size, color, texture);
}

func void draw_texture_center_camera(s_v2 pos, s_v2 size, s_rgb color, SDL_Texture* texture, s_camera camera)
{
	pos.x -= size.x * 0.5f;
	pos.y -= size.y * 0.5f;
	pos = world_to_screen(pos, camera);
	draw_texture_topleft(pos, size, color, texture);
}

func void draw_texture_topleft(s_v2 pos, s_v2 size, s_rgb color, SDL_Texture* texture)
{
	SDL_FRect rect;
	rect.x = pos.x;
	rect.y = pos.y;
	rect.w = size.x;
	rect.h = size.y;
	SDL_SetTextureColorModFloat(texture, color.r, color.g, color.b);
	SDL_RenderTexture(g_renderer, texture, null, &rect);
}

func s_v2 v2(f32 x, f32 y)
{
	return (s_v2){x, y};
}

func s_v2 v2_1(f32 v)
{
	return (s_v2){v, v};
}

func void v2_add_p(s_v2* out, s_v2 to_add)
{
	out->x += to_add.x;
	out->y += to_add.y;
}

func int make_entity(bool* active_arr, s_entity_index_data* index_data)
{
	for(int i = 0; i < c_max_entities; i += 1) {
		if(active_arr[i]) { continue; }
		active_arr[i] = true;
		index_data->min_index = min(index_data->min_index, i);
		index_data->max_index_plus_one = max(index_data->max_index_plus_one, i + 1);
		return i;
	}
	assert(false);
	return -1;
}

func int make_projectile(s_v2 pos, s_v2 size, s_v2 dir, SDL_Texture* texture)
{
	int entity = make_entity(g_game.projectile_arr.active, &g_game.projectile_arr.index_data);
	g_game.projectile_arr.pos[entity] = pos;
	g_game.projectile_arr.prev_pos[entity] = pos;
	g_game.projectile_arr.dir[entity] = dir;
	g_game.projectile_arr.speed[entity] = 8;
	g_game.projectile_arr.ticks_left[entity] = 500;
	g_game.projectile_arr.texture[entity] = texture;
	g_game.projectile_arr.gravity[entity] = 0;
	g_game.projectile_arr.size[entity] = size;
	return entity;
}

func int make_aoe(s_v2 pos, s_v2 size, SDL_Texture* texture)
{
	int entity = make_entity(g_game.aoe_arr.active, &g_game.aoe_arr.index_data);
	g_game.aoe_arr.pos[entity] = pos;
	g_game.aoe_arr.prev_pos[entity] = pos;
	g_game.aoe_arr.speed[entity] = 1;
	g_game.aoe_arr.ticks_left[entity] = 500;
	g_game.aoe_arr.texture[entity] = texture;
	g_game.aoe_arr.hit_timer[entity] = 0;
	g_game.aoe_arr.size[entity] = size;
	g_game.aoe_arr.falling[entity] = false;
	return entity;
}

func int make_enemy(s_v2 pos, e_enemy_type type)
{
	int entity = make_entity(g_game.enemy_arr.active, &g_game.enemy_arr.index_data);
	g_game.enemy_arr.type[entity] = type;
	g_game.enemy_arr.pos[entity] = pos;
	g_game.enemy_arr.prev_pos[entity] = pos;
	g_game.enemy_arr.speed[entity] = 0.33f;
	g_game.enemy_arr.damage_taken[entity] = 0;
	g_game.enemy_arr.max_health[entity] = get_enemy_type_property(entity, max_health);

	g_game.next_entity_id += 1;
	g_game.enemy_arr.id[entity] = g_game.next_entity_id;
	return entity;
}

func int make_pickup(s_v2 pos, e_pickup type)
{
	int entity = make_entity(g_game.inactive_pickup_arr.active, &g_game.inactive_pickup_arr.index_data);
	g_game.inactive_pickup_arr.pos[entity] = pos;
	g_game.inactive_pickup_arr.type[entity] = type;
	return entity;
}

func int make_exp(s_v2 pos)
{
	int entity = make_pickup(pos, e_pickup_exp0);
	g_game.inactive_pickup_arr.exp_to_give[entity] = 1;
	return entity;
}

func f32 v2_length(s_v2 v)
{
	f32 length = v.x * v.x + v.y * v.y;
	return sqrtf(length);
}

func s_v2 v2_normalized(s_v2 v)
{
	f32 length = v2_length(v);
	if(length != 0) {
		v.x /= length;
		v.y /= length;
	}
	return v;
}

func s_v2 v2_scale(s_v2 out, f32 scale)
{
	out.x *= scale;
	out.y *= scale;
	return out;
}

func void v2_scale_p(s_v2* out, f32 scale)
{
	out->x *= scale;
	out->y *= scale;
}

func void v2_add_scale_p(s_v2* out, s_v2 to_add, f32 scale)
{
	v2_scale_p(&to_add, scale);
	v2_add_p(out, to_add);
}

func f32 lerp(f32 a, f32 b, f32 t)
{
	return a + (b - a) * t;
}

func s_v2 lerp_v2(s_v2 a, s_v2 b, f32 t)
{
	a.x = lerp(a.x, b.x, t);
	a.y = lerp(a.y, b.y, t);
	return a;
}

func SDL_Texture* load_texture(char* path)
{
	int x, y, n;
	u8* data = stbi_load(path, &x, &y, &n, 4);
	assert(data);
	SDL_Surface* surface = SDL_CreateSurfaceFrom(x, y, SDL_PIXELFORMAT_ABGR8888, data, x * 4);

	assert(surface);
	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_renderer, surface);
	assert(texture);
	SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
	SDL_DestroySurface(surface);
	stbi_image_free(data);
	return texture;
}

// func bool for_every_player(s_entity_iterator* it)
// {
// 	for(int i = it->next_index; i < g_game.max_index_plus_one; i += 1) {
// 		it->index = i;
// 		it->next_index = i + 1;
// 		if(!g_game.active_arr[i]) { continue; }
// 		u64 wanted = e_component_flag_player;
// 		u64 flags = g_game.component_mask_arr[i];
// 		if((flags & wanted) == wanted) { return true; }
// 	}
// 	return false;
// }

func s_v2 v2_sub(s_v2 a, s_v2 b)
{
	a.x -= b.x;
	a.y -= b.y;
	return a;
}

func s_v2 v2_from_to(s_v2 from, s_v2 to)
{
	s_v2 v = v2_sub(to, from);
	return v;
}

func s_v2 v2_from_to_normalized(s_v2 from, s_v2 to)
{
	s_v2 v = v2_from_to(from, to);
	return v2_normalized(v);
}

func void add_at_most_int(int* out, int to_add, int max_val)
{
	*out += to_add;
	if(*out > max_val) { *out = max_val; }
}

func f32 v2_length_squared(s_v2 v)
{
	return v.x * v.x + v.y * v.y;
}

func f32 v2_distance_squared(s_v2 a, s_v2 b)
{
	return v2_length_squared(v2_sub(a, b));
}

func f32 v2_distance(s_v2 a, s_v2 b)
{
	return sqrtf(v2_distance_squared(a, b));
}

func s_rgb make_color_1(float v)
{
	return (s_rgb){v, v, v};
}

func s_rgb make_color(float r, float g, float b)
{
	return (s_rgb){r, g, b};
}

func float get_game_speed(void)
{
	f32 speed = c_game_speed_arr[g_game.speed_index];
	if(g_game.state0 == e_state0_play && g_game.state1 == e_state1_level_up) {
		speed = 0;
	}
	return speed;
}

func int circular_index(int index, int size)
{
	assert(size > 0);
	if(index >= 0) {
		return index % size;
	}
	return (size - 1) - ((-index - 1) % size);
}

func void circular_index_add(int* index, int to_add, int size)
{
	*index = circular_index(*index + to_add, size);
}

func f64 at_most_f64(f64 a, f64 b)
{
	return a > b ? b : a;
}

func bool is_on_screen_center(s_v2 pos, s_v2 size)
{
	s_v2 half_size = v2_scale(size, 0.5f);
	if(pos.x + half_size.x < 0) { return false; }
	if(pos.y + half_size.y < 0) { return false; }
	if(pos.x - half_size.x > c_window_width) { return false; }
	if(pos.y - half_size.y > c_window_height) { return false; }
	return true;
}

func s_v2 v2_sub_scale(s_v2 a, s_v2 b, float scale)
{
	a.x -= b.x * scale;
	a.y -= b.y * scale;
	return a;
}

func bool rect_vs_rect_topleft(s_v2 pos0, s_v2 size0, s_v2 pos1, s_v2 size1)
{
	return pos0.x + size0.x >= pos1.x &&
		pos0.y + size0.y >= pos1.y &&
		pos0.x <= pos1.x + size1.x &&
		pos0.y <= pos1.y + size1.y;
}

func bool rect_vs_rect_center(s_v2 pos0, s_v2 size0, s_v2 pos1, s_v2 size1)
{
	return rect_vs_rect_topleft(v2_sub_scale(pos0, size0, 0.5f), size0, v2_sub_scale(pos1, size1, 0.5f), size1);
}

func void remove_entity(int entity, bool* active_arr, s_entity_index_data* index_data)
{
	assert(active_arr[entity]);
	active_arr[entity] = false;
	if(entity == index_data->min_index) {
		index_data->min_index += 1;
	}
	if(entity + 1 == index_data->max_index_plus_one) {
		index_data->max_index_plus_one -= 1;
	}
}

func int add_exp(s_player* player, int exp_to_give)
{
	int result = 0;
	player->curr_exp += exp_to_give;
	int exp_to_level = get_exp_to_level(player->level);
	while(player->curr_exp >= exp_to_level) {
		player->level += 1;
		player->curr_exp -= exp_to_level;
		exp_to_level = get_exp_to_level(player->level);
		result += 1;
	}
	g_game.level_up_triggers += result;
	return result;
}

func s_v2 wxy(f32 x, f32 y)
{
	return v2(c_window_size.x * x, c_window_size.y * y);
}

func void draw_rect_topleft(s_v2 pos, s_v2 size, s_rgb color)
{
	SDL_FRect rect;
	rect.x = pos.x;
	rect.y = pos.y;
	rect.w = size.x;
	rect.h = size.y;

	SDL_SetRenderDrawColorFloat(g_renderer, color.r, color.g, color.b, 1);
	SDL_RenderFillRect(g_renderer, &rect);
}

func void draw_rect_center_camera(s_v2 pos, s_v2 size, s_rgb color, s_camera camera)
{
	pos.x -= size.x * 0.5f;
	pos.y -= size.y * 0.5f;
	draw_rect_topleft_camera(pos, size, color, camera);
}

func void draw_rect_topleft_camera(s_v2 pos, s_v2 size, s_rgb color, s_camera camera)
{
	pos = world_to_screen(pos, camera);
	draw_rect_topleft(pos, size, color);
}

func int get_exp_to_level(int level)
{
	int exp_to_level = 5 + (level - 1) * 3;
	return exp_to_level;
}

func void on_failed_assert(int line)
{
	printf("FAILED ASSERT AT %i\n", line);
	*((int*)0) = 0;
}

func s_v2 get_center(s_v2 pos, s_v2 size)
{
	pos.x += size.x * 0.5f;
	pos.y += size.y * 0.5f;
	return pos;
}

func s_pos_area make_pos_area(s_v2 pos, s_v2 size, s_v2 element_size, float spacing, int count, int flags)
{
	// @Note(tkap, 25/09/2024): -1 means we don't care about the count
	assert(count > 0 || count == -1);
	s_pos_area area = zero;
	s_v2 center_pos = get_center(pos, size);
	s_v2 space_used = v2(0, 0);
	bool center_x = (flags & e_pos_area_flag_center_x) != 0;
	bool center_y = (flags & e_pos_area_flag_center_y) != 0;
	bool horizontal = (flags & e_pos_area_flag_vertical) == 0;

	space_used.x = element_size.x * count;
	space_used.x += spacing * (count - 1);

	space_used.y = element_size.y * count;
	space_used.y += spacing * (count - 1);

	if(center_x) {
		if(horizontal) {
			area.pos.x = center_pos.x - space_used.x * 0.5f;
		}
		else {
			area.pos.x = center_pos.x - element_size.x * 0.5f;
		}
	}
	else {
		area.pos.x = pos.x;
	}

	if(center_y) {
		if(horizontal) {
			area.pos.y = center_pos.y - element_size.y * 0.5f;
		}
		else {
			area.pos.y = center_pos.y - space_used.y * 0.5f;
		}
	}
	else {
		area.pos.y = pos.y;
	}

	if(horizontal) {
		area.advance.x = element_size.x + spacing;
	}
	else {
		area.advance.y = element_size.x + spacing;
	}

	return area;
}

func s_v2 pos_area_get_advance(s_pos_area* area)
{
	s_v2 result = area->pos;
	area->pos.x += area->advance.x;
	area->pos.y += area->advance.y;
	return result;
}

func void add_weapon(s_player* player, e_weapon weapon_index)
{
	assert(player->weapon_count < e_weapon_count);
	s_weapon weapon = zero;
	weapon.level = 1;
	player->weapon_arr[weapon_index] = weapon;
	player->weapon_index_arr[player->weapon_count] = weapon_index;
	player->weapon_count += 1;
}

func s_v2 rand_v2(s_rng* rng)
{
	s_v2 v;
	v.x = randf_negative32(rng);
	v.y = randf_negative32(rng);
	return v;
}

func s_v2 rand_v2_normalized(s_rng* rng)
{
	s_v2 v = rand_v2(rng);
	return v2_normalized(v);
}

func f32 at_most_f32(float a, float b)
{
	return a > b ? b : a;
}

func f32 get_projectile_angle(int projectile_i, int num_projectiles)
{
	if(num_projectiles <= 1) {
		return 0;
	}
	int c_projectiles_per_full_circle = 20;
	f32 spread = at_most_f32(1.0f, (f32)num_projectiles / c_projectiles_per_full_circle);
	f32 angle = (f32)projectile_i / (num_projectiles - 1) * spread;
	angle -= spread / 2;
	angle *= c_tau;
	return angle;
}

func s_v2 v2_rotated(s_v2 v, f32 angle)
{
	f32 x = v.x;
	f32 y = v.y;

	f32 cs = cosf(angle);
	f32 sn = sinf(angle);

	v.x = x * cs - y * sn;
	v.y = x * sn + y * cs;

	return v;
}

func int get_closest_enemy(s_v2 pos)
{
	int closest = -1;
	f32 smallest_distance = 99999999999.0f;

	for_enemy_partial(enemy) {
		if(!g_game.enemy_arr.active[enemy]) { continue; }
		float distance = v2_distance_squared(pos, g_game.enemy_arr.pos[enemy]);
		if(distance < smallest_distance) {
			smallest_distance = distance;
			closest = enemy;
		}
	}
	return closest;
}

func bool is_enemy_already_hit(s64* arr, int count, int enemy)
{
	for(int i = 0; i < count; i += 1) {
		if(arr[i] == g_game.enemy_arr.id[enemy]) { return true; }
	}
	return false;
}

func s_v2 get_enemy_size(int enemy)
{
	s_v2 size = get_enemy_type_property(enemy, size);
	return size;
}

func int ceilfi(float x)
{
	return (int)ceilf(x);
}

func int floorfi(float x)
{
	return (int)floorf(x);
}

func s_v2 world_to_screen(s_v2 pos, s_camera camera)
{
	pos.x -= camera.pos.x;
	pos.y -= camera.pos.y;
	return pos;
}

func void v2_sub_p(s_v2* out, s_v2 to_sub)
{
	out->x -= to_sub.x;
	out->y -= to_sub.y;
}

func void get_camera_bounds(s_v2* out_min, s_v2* out_max, s_camera camera)
{
	out_min->x = camera.pos.x;
	out_min->y = camera.pos.y;
	out_max->x = camera.pos.x + c_window_size.x;
	out_max->y = camera.pos.y + c_window_size.y;
}

func s_linear_arena make_linear_arena_alloc(int size)
{
	s_linear_arena arena = zero;
	arena.capacity = size;
	arena.memory = malloc(size);
	return arena;
}

func void* make_dynamic_array(int element_size, int initial_capacity, s_linear_arena* arena)
{
	u8* data = arena_get(element_size * initial_capacity + sizeof(s_dynamic_array_data), arena);
	s_dynamic_array_data* dynamic_data = (s_dynamic_array_data*)data;
	dynamic_data->count = 0;
	dynamic_data->capacity = initial_capacity;
	dynamic_data->element_size = element_size;
	void* result = data + sizeof(s_dynamic_array_data);
	return result;
}

// @Note(tkap, 27/09/2024): This function crashes every now and then on "long" (20000+ enemies spawned) play sessions, but I'm too lazy to debug it
func void array_add(void** in_arr, void* new_element, s_linear_arena* arena)
{
	u8* arr = *in_arr;
	s_dynamic_array_data* dynamic_data = (s_dynamic_array_data*)(arr - sizeof(s_dynamic_array_data));
	if(dynamic_data->count == dynamic_data->capacity) {
		int new_capacity = dynamic_data->capacity * 2;
		int old_capacity = dynamic_data->capacity;

		u8* new_ptr = arena_get(new_capacity * dynamic_data->element_size + sizeof(s_dynamic_array_data), arena);
		s_dynamic_array_data* new_header_ptr = (s_dynamic_array_data*)(new_ptr - sizeof(s_dynamic_array_data));

		memcpy(new_ptr, arr, old_capacity * dynamic_data->element_size);
		*new_header_ptr = *dynamic_data;

		*in_arr = new_ptr;
		arr = new_ptr;
		dynamic_data = new_header_ptr;
		dynamic_data->capacity = new_capacity;
	}
	memcpy(arr + dynamic_data->count * dynamic_data->element_size, new_element, dynamic_data->element_size);
	dynamic_data->count += 1;
}

func int array_get_count(void* in_arr)
{
	u8* arr = in_arr;
	s_dynamic_array_data* dynamic_data = (s_dynamic_array_data*)(arr - sizeof(s_dynamic_array_data));
	return dynamic_data->count;
}

func void* arena_get(int bytes, s_linear_arena* arena)
{
	bytes = (bytes + 7) & ~7;
	assert(arena->used + bytes <= arena->capacity);
	void* result = arena->memory + arena->used;
	arena->used += bytes;
	return result;
}

func s_v2 v2_add(s_v2 a, s_v2 b)
{
	a.x += b.x;
	a.y += b.y;
	return a;
}

func int id_to_enemy(s_entity_id id)
{
	s_enemy_arr* enemy_arr = &g_game.enemy_arr;
	assert(id.id > 0);
	assert(id.index >= 0);

	if(enemy_arr->id[id.index] != id.id) { return -1; }
	if(!enemy_arr->active[id.index]) { return -1; }
	return id.index;
}

func int id_to_inactive_pickup(s_entity_id id)
{
	s_inactive_pickup_arr* inactive_pickup_arr = &g_game.inactive_pickup_arr;
	assert(id.index >= 0);

	if(!inactive_pickup_arr->active[id.index]) { return -1; }
	return id.index;
}

func int id_to_active_pickup(s_entity_id id)
{
	s_active_pickup_arr* active_pickup_arr = &g_game.active_pickup_arr;
	assert(id.index >= 0);

	if(!active_pickup_arr->active[id.index]) { return -1; }
	return id.index;
}

func bool circle_vs_rect_center(s_v2 center, float radius, s_v2 rect_pos, s_v2 rect_size)
{
	bool collision = false;

	f32 dx = fabsf(center.x - rect_pos.x);
	f32 dy = fabsf(center.y - rect_pos.y);

	if(dx > (rect_size.x/2.0f + radius)) { return false; }
	if(dy > (rect_size.y/2.0f + radius)) { return false; }

	if(dx <= (rect_size.x/2.0f)) { return true; }
	if(dy <= (rect_size.y/2.0f)) { return true; }

	f32 cornerDistanceSq = (dx - rect_size.x/2.0f)*(dx - rect_size.x/2.0f) +
		(dy - rect_size.y/2.0f)*(dy - rect_size.y/2.0f);

	collision = (cornerDistanceSq <= (radius*radius));

	return collision;
}

func s_v2 v2_from_angle(float angle)
{
	return v2(cosf(angle), sinf(angle));
}

func s_v2 move_towards_v2(s_v2 a, s_v2 b, float speed, float* out_distance)
{
	s_v2 dir = v2_from_to(a, b);
	s_v2 dir_n = v2_normalized(dir);
	float distance = v2_length(dir);
	v2_scale_p(&dir_n, at_most_f32(distance, speed));
	s_v2 result = v2_add(a, dir_n);
	if(out_distance) {
		*out_distance = v2_distance(result, b);
	}
	return result;
}

func int activate_pickup(int entity)
{
	s_inactive_pickup_arr* inactive_pickup_arr = &g_game.inactive_pickup_arr;
	s_active_pickup_arr* active_pickup_arr = &g_game.active_pickup_arr;
	int active = make_entity(active_pickup_arr->active, &active_pickup_arr->index_data);
	active_pickup_arr->exp_to_give[active] = inactive_pickup_arr->exp_to_give[entity];
	active_pickup_arr->type[active] = inactive_pickup_arr->type[entity];
	active_pickup_arr->pos[active] = inactive_pickup_arr->pos[entity];
	active_pickup_arr->prev_pos[active] = inactive_pickup_arr->pos[entity];
	active_pickup_arr->speed[active] = 1;
	return active;
}

func int activate_pickup_and_remove(int entity)
{
	s_inactive_pickup_arr* inactive_pickup_arr = &g_game.inactive_pickup_arr;
	assert(inactive_pickup_arr->active[entity]);

	int active = activate_pickup(entity);
	remove_entity(entity, inactive_pickup_arr->active, &inactive_pickup_arr->index_data);
	return active;
}

func bool damage_enemy(int enemy, int damage)
{
	s_enemy_arr* enemy_arr = &g_game.enemy_arr;
	enemy_arr->damage_taken[enemy] += damage;
	enemy_arr->last_hit_time[enemy] = multiplied_render_time;
	if(enemy_arr->damage_taken[enemy] >= enemy_arr->max_health[enemy]) {
		make_exp(enemy_arr->pos[enemy]);
		remove_entity(enemy, enemy_arr->active, &enemy_arr->index_data);
		if(rand_chance100(&g_game.rng, 1)) {
			make_pickup(v2_add(enemy_arr->pos[enemy], v2(10, 0)), e_pickup_vacuum_exp);
		}
		return true;
	}
	return false;
}

func int make_visual_effect(s_v2 pos, e_visual_effect type)
{
	s_visual_effect_arr* visual_effect_arr = &g_game.visual_effect_arr;
	int entity = make_entity(visual_effect_arr->active, &visual_effect_arr->index_data);
	visual_effect_arr->pos[entity] = pos;
	visual_effect_arr->time_passed[entity] = 0;
	visual_effect_arr->type[entity] = type;
	return entity;
}

func f32 index_count_safe_div(int i, int count)
{
	int count2 = count - 1;
	if(count2 == 0) { return 1; }
	return i / (f32)count2;
}

func int at_least_int(int a, int b)
{
	return max(a, b);
}

func int get_weapon_delay(s_weapon weapon, e_weapon type)
{
	int result = g_weapon_data_arr[type].delay;

	switch(type) {
		case e_weapon_lightning_bolt: {
			result -= weapon.level * 2;
		} break;
	}

	return at_least_int(0, result);
}