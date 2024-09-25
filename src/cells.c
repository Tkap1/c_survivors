
func s_cell_iterator make_cell_iterator()
{
	s_cell_iterator it = zero;
	it.x = -1;
	it.y = -1;
	return it;
}

func bool get_enemy_in_cells(s_cell_iterator* it, s_v2 pos, s_v2 size)
{
	s_enemy_arr* enemy_arr = &g_game.enemy_arr;

	int x_index = floorfi((pos.x - g_game.enemy_cell_min_bounds.x) / c_cell_size);
	int y_index = floorfi((pos.y - g_game.enemy_cell_min_bounds.y) / c_cell_size);

	for(int y = it->y; y <= 1; y += 1) {
		for(int x = it->x; x <= 1; x += 1) {
			int xx = x_index + x;
			int yy = y_index + y;
			if(xx < 0 || xx >= c_max_cells) { return false; }
			if(yy < 0 || yy >= c_max_cells) { return false; }
			s_entity_id* arr = g_game.enemy_cell_arr[yy][xx];
			int count = array_get_count(arr);
			for(int id_i = it->id_index; id_i < count; id_i += 1) {
				it->id_index = id_i + 1;
				int enemy = id_to_enemy(arr[id_i]);
				if(enemy >= 0) {
					if(rect_vs_rect_center(pos, size, enemy_arr->pos[enemy], get_enemy_type_property(enemy, size))) {
						it->index = enemy;
						return true;
					}
				}
			}
			it->id_index = 0;
			it->x = x + 1;
		}
		it->y = y + 1;
		it->x = -1;
	}
	return false;
}

func bool get_inactive_pickup_in_cells(s_cell_iterator* it, s_v2 pos, float radius)
{
	s_inactive_pickup_arr* inactive_pickup_arr = &g_game.inactive_pickup_arr;

	int x_index = floorfi((pos.x - g_game.inactive_pickup_cell_min_bounds.x) / c_cell_size);
	int y_index = floorfi((pos.y - g_game.inactive_pickup_cell_min_bounds.y) / c_cell_size);

	for(int y = it->y; y <= 1; y += 1) {
		for(int x = it->x; x <= 1; x += 1) {
			int xx = x_index + x;
			int yy = y_index + y;
			if(xx < 0 || xx >= c_max_cells) { return false; }
			if(yy < 0 || yy >= c_max_cells) { return false; }
			s_entity_id* arr = g_game.inactive_pickup_cell_arr[yy][xx];
			int count = array_get_count(arr);
			for(int id_i = it->id_index; id_i < count; id_i += 1) {
				it->id_index = id_i + 1;
				int inactive_pickup = id_to_inactive_pickup(arr[id_i]);
				if(inactive_pickup >= 0) {
					if(circle_vs_rect_center(pos, radius, inactive_pickup_arr->pos[inactive_pickup], c_exp_size)) {
						it->index = inactive_pickup;
						return true;
					}
				}
			}
			it->id_index = 0;
			it->x = x + 1;
		}
		it->y = y + 1;
		it->x = -1;
	}
	return false;
}

func bool get_active_pickup_in_cells(s_cell_iterator* it, s_v2 pos, s_v2 size)
{
	s_active_pickup_arr* active_pickup_arr = &g_game.active_pickup_arr;

	int x_index = floorfi((pos.x - g_game.active_pickup_cell_min_bounds.x) / c_cell_size);
	int y_index = floorfi((pos.y - g_game.active_pickup_cell_min_bounds.y) / c_cell_size);

	for(int y = it->y; y <= 1; y += 1) {
		for(int x = it->x; x <= 1; x += 1) {
			int xx = x_index + x;
			int yy = y_index + y;
			if(xx < 0 || xx >= c_max_cells) { return false; }
			if(yy < 0 || yy >= c_max_cells) { return false; }
			s_entity_id* arr = g_game.active_pickup_cell_arr[yy][xx];
			int count = array_get_count(arr);
			for(int id_i = it->id_index; id_i < count; id_i += 1) {
				it->id_index = id_i + 1;
				int active_pickup = id_to_active_pickup(arr[id_i]);
				if(active_pickup >= 0) {
					if(rect_vs_rect_center(pos, size, active_pickup_arr->pos[active_pickup], c_exp_size)) {
						it->index = active_pickup;
						return true;
					}
				}
			}
			it->id_index = 0;
			it->x = x + 1;
		}
		it->y = y + 1;
		it->x = -1;
	}
	return false;
}