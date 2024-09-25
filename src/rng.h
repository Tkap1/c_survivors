

typedef struct s_rng
{
	u64 seed;
	// f64 randf64()
	// {
	// 	f64 result = (f64)randu() / (f64)4294967295;
	// 	return result;
	// }

	// float randf32()
	// {
	// 	return (float)randu() / (float)4294967295;
	// }

	// f64 randf64_11()
	// {
	// 	return randf64() * 2 - 1;
	// }

	// f32 randf32_11()
	// {
	// 	return randf32() * 2 - 1;
	// }

	// u64 randu64()
	// {
	// 	return (u64)(randf64() * (f64)c_max_u64);
	// }


	// // min inclusive, max inclusive
	// int rand_range_ii(int min, int max)
	// {
	// 	if(min > max)
	// 	{
	// 		int temp = min;
	// 		min = max;
	// 		max = temp;
	// 	}

	// 	return min + (randu() % (max - min + 1));
	// }

	// // min inclusive, max exclusive
	// int rand_range_ie(int min, int max)
	// {
	// 	if(min > max)
	// 	{
	// 		int temp = min;
	// 		min = max;
	// 		max = temp;
	// 	}

	// 	return min + (randu() % (max - min));
	// }

	// float randf_range(float min_val, float max_val)
	// {
	// 	if(min_val > max_val)
	// 	{
	// 		float temp = min_val;
	// 		min_val = max_val;
	// 		max_val = temp;
	// 	}

	// 	float r = randf32();
	// 	return min_val + (max_val - min_val) * r;
	// }

	// b8 chance100(float chance)
	// {
	// 	assert(chance >= 0);
	// 	// @Note(tkap, 15/06/2023): This assert is probably not worth having because of our debug stat menu thing. We will easily go above 100%
	// 	// when testing, so this will get in the way. We do not want to pay the price of clamping only for debug reasons.
	// 	// assert(chance <= 100);
	// 	b8 result = chance / 100 >= randf64();
	// 	return result;
	// }

	// b8 chance1(f64 chance)
	// {
	// 	assert(chance >= 0);
	// 	return chance >= randf64();
	// }

	// b8 rand_bool()
	// {
	// 	return (b8)(randu() & 1);
	// }

	// int while_chance1(float chance)
	// {
	// 	int result = 0;
	// 	while(chance1(chance)) {
	// 		result += 1;
	// 	}
	// 	return result;
	// }

} s_rng;
