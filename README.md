it's a thing I guess
Also, this program has been tested on x86_64-linux and aarch64-macos. The prompt (REPL) definitely won't work on Windows, and the rest of the program hasn't been tested (and I can't be bothered to check for anything POSIX-specific).

# documentation
Not much, but there is a syntax guide [here](docs/README.md) if you want to read it.

# building
```sh
git submodule update --init # first time
make
```
Then run `build/mml --help` to display the command-line options.
## NEW!
If you have `zig` installed, you can use `zig build` to build the library and executable. Here are the commands:
```sh
git submodule update --init # first time
zig build
```
You can run `zig build -Doptimize=ReleaseFast` to get more optimization. `zig build` also allows
easy cross-compilation; [this](https://zig.guide/build-system/cross-compilation/) page has some more information (should still be up to date-ish).
This will automatically build the static library as well (every source file except `src/main.c` and `src/prompt.c`).

# library documentation
It's not much of a library, but it is built to be easily extendable (hopefully that's true).
A minimal example:
```c
#include <stdint.h>

#include "mml/mml.h"

int32_t main(void)
{
	MML_state *state = MML_init_state();

	const char *s = "cos{1.5pi} == 0.0";
	MML_Value ret = MML_eval_parse(state, s);
	MML_println_typedval(state, &ret);

	MML_cleanup_state(state);

	return 0;
}
```

And it can be compiled with this command (assuming you've run `make shared_lib` or `make static_lib`, are currently in the root directory, and named the example file `test.c`):
```sh
gcc -o test test.c -Iincl -Lbuild -lmml -lm
```
The shared object file is a bit finnicky, so if you link with the shared library, you'll probably have to do this (the above should work on OS X, though):
```sh
gcc -o test test.c -Iincl build/libmml.so
```
