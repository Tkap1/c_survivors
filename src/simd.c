

func __m128 simd_distance_squared(__m128 x0, __m128 y0, __m128 x1, __m128 y1)
{
	__m128 x2 = _mm_sub_ps(x0, x1);
	__m128 y2 = _mm_sub_ps(y0, y1);
	__m128 x3 = _mm_mul_ps(x2, x2);
	__m128 y3 = _mm_mul_ps(y2, y2);
	return _mm_add_ps(x3, y3);
}

func float simd_min(__m128 x, int* out_index)
{
	int index = 0;
	f32 result = x.m128_f32[0];
	for(int i = 0; i < 4; i += 1) {
		if(x.m128_f32[index] > x.m128_f32[i]) {
			index = i;
			result = x.m128_f32[index];
		}
	}
	*out_index = index;
	return result;
}


// bad place for this

func bool floats_equal(float a, float b)
{
	return (a >= b - epsilon && a <= b + epsilon);
}


func float clamp(float val, float min_val, float max_val)
{
	return min(max_val, max(val, min_val));
}

func float ilerp(float start, float end, float value)
{
	if(floats_equal(start, end)) { return 0; }
	float x = (value - start) / (end - start);
	return x;
}

func float ilerp_clamp(float start, float end, float value)
{
	return ilerp(start, end, clamp(value, start, end));
}

func float handle_advanced_easing(float x, float x_start, float x_end)
{
	x = clamp(ilerp_clamp(x_start, x_end, x), 0.0f, 1.0f);
	return x;
}

func float ease_in_expo(float x)
{
	if(floats_equal(x, 0)) { return 0; }
	return powf(2, 10 * x - 10);
}

func float ease_linear(float x)
{
	return x;
}

func float ease_in_quad(float x)
{
	return x * x;
}

func float ease_out_quad(float x)
{
	float x2 = 1 - x;
	return 1 - x2 * x2;
}

func float ease_out_expo(float x)
{
	if(floats_equal(x, 1)) { return 1; }
	return 1 - powf(2, -10 * x);
}

func float ease_out_elastic(float x)
{
	float c4 = (2 * c_pi) / 3;
	if(floats_equal(x, 0) || floats_equal(x, 1)) { return x; }
	return powf(2, -5 * x) * sinf((x * 5 - 0.75f) * c4) + 1;
}

func float ease_out_elastic2(float x)
{
	float c4 = (2 * c_pi) / 3;
	if(floats_equal(x, 0) || floats_equal(x, 1)) { return x; }
	return powf(2, -10 * x) * sinf((x * 10 - 0.75f) * c4) + 1;
}

func float ease_out_back(float x)
{
	float c1 = 1.70158f;
	float c3 = c1 + 1;
	return 1 + c3 * powf(x - 1, 3) + c1 * powf(x - 1, 2);
}

#define m_advanced_easings \
X(ease_linear) \
X(ease_in_expo) \
X(ease_in_quad) \
X(ease_out_quad) \
X(ease_out_expo) \
X(ease_out_elastic) \
X(ease_out_elastic2) \
X(ease_out_back) \

#define X(name) \
func float name##_advanced(float x, float x_start, float x_end, float target_start, float target_end) \
{ \
	x = handle_advanced_easing(x, x_start, x_end); \
	return lerp(target_start, target_end, name(x)); \
}
m_advanced_easings
#undef X
