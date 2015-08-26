# `89 ec 5d c3` - Underhanded C 2015 Entry
Entry in the [2015 underhanded C contest][uc].

See [this post](http://blog.mattbierner.com/underhanded-c-2015/) for all the details on how this works.

# Usage
The actual entry is contained in `match.h` which contains the entire underhanded implementation of the `match` function. `main.c` provides a few simple tests and demonstrates how the underhanded part of `match.h` can be triggered. It is not part of the main entry. 

## Compiling
This code is written in c99 for gcc. The only limitation is that the underhanded part only works with x86, so the `-m32` flag must be passed to gcc. Targeting x86 machines is more of a design decision. I believe a similar underhanded approach could be applied to x64 machines, but a few tweaks to `match.c` would be required. More on that later.
 
To compile `main.c` and run the basic tests:

```bash
gcc -m32 -std=c99 main.c -o main ; ./main
```

## TLDR
Here's what happens end-to-end in the underhanded implementation:

1. The programmer somehow sets `FE_INVALID`. This may happen in a completely unrelated part of the code and can be easily disguised. 
2. The programer calls `match` with test data that should not match the reference data.
3. `match` calls `do_elements_match` for each element in the data. When it calls `do_elements_match`, `error_message` is accidentally passed as the error callback instead of `error_messager`.
4. In `do_elements_match`, the `FE_INVALID` flag is now set. We see that function will `return 0;` in this case.
5. In the error conditional, `do_elements_match` invokes `on_error`. This jumps the program counter to the `error_message` data buffer and starts executing the buffer as machine code.
6. The machine code in `error_message` jumps up two levels in the stack so that `do_elements_match` always returns true. `return 0` in `do_elements_match` is never even reached.
7. `match` sees that `do_elements_match` returned `1`. The elements must have matched so it happily continues on. 
8. Amazingly, it turns out that all elements match. `match` returns `1`.

## Sneaky Parts
Here are a few  sneaky parts

1. Code is short and simple (less than 20 lines of very clear logic).
2. Code is well documented. Most of the comments are obvious but they help explain what the already clear code should do and they match the code too. 
3. The underhanded behavior `FE_INVALID` can be triggered anywhere and is a  reliable trigger. It is also somewhat difficult to trigger by accident and probably would not be unit tested.
4. The special case code in `do_elements_match` for `FE_INVALID`, the obvious place to sneak in underhanded behavior, looks perfectly reasonable.
5. `error_label` looks like a valid localization (provided that you don't know Chinese).
6. Treating a string as machine code is not expected behavior.
7. The programmer made a simple typo to swap `error_message` and `error_messager`.
8. No compiler warnings or errors are generated.

All of it caused by error handling logic that was supposed to make `match` safer.


[uc]: http://www.underhanded-c.org/_page_id_5.html
[fenv]: http://en.cppreference.com/w/c/numeric/fenv