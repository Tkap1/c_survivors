
// @Note(tkap, 12/06/2024): https://www.pcg-random.org/download.html
#pragma warning(push, 0)
func u32 randu(s_rng* rng)
{
	uint64_t oldstate = rng->seed;
	// Advance internal state
	rng->seed = oldstate * 6364136223846793005ULL + 1;
	// Calculate output function (XSH RR), uses old state for max ILP
	uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
	uint32_t rot = oldstate >> 59u;
	return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
#pragma warning(pop)

func s_rng make_rng(u64 seed)
{
	s_rng rng = zero;
	randu(&rng);
	rng.seed += seed;
	randu(&rng);
	return rng;
}

// min inclusive, max exclusive
func int rand_range_ie(s_rng* rng, int min, int max)
{
	if(min > max) {
		int temp = min;
		min = max;
		max = temp;
	}

	return min + (randu(rng) % (max - min));
}

func f32 randf32(s_rng* rng)
{
	return (f32)randu(rng) / (f32)4294967295;
}

func f32 randf_negative32(s_rng* rng)
{
	return randf32(rng) * 2 - 1;
}

func bool rand_chance100(s_rng* rng, float chance)
{
	float r = randf32(rng) * 100;
	return r <= chance;
}