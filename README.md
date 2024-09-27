VERY incomplete Vampire Survivors clone using C and SDL3.

Run `main.exe` or build it by calling one of:

    	build.bat
    	build.bat optimized_debug
    	build.bat release

WASD movement. Numpad +/- to change game speed

I made this because I wanted to see if I could tolerate the lack of features compared to C++ in exhange for very low compilation times. I couldn't. The main reason is the lack of the type of array/list I like to use: static size known at compile-time, bounds-checked, and being able to use the subscript operator without any indirection (arr[0] vs arr.data[0]). As far as I know this is not possible in C (maybe with some macro magic?)

Here is a list of other things that I miss but could possibly live without:

Operator overloading (vector math mainly)

Function overloading

Methods (mostly for lists e.g: foos.add(foo) vs array_add(&foos, foo))

static_assert

I prefer constexpr rather than #define

Destructors (only for profiling, so I can slap a `timed_function()` at the start of a function, rather than `start_timing` and `end_timing`)

Default arguments