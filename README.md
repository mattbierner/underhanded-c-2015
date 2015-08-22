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

```
gcc -m32 -std=c99 main.c  -o main ; ./main
```


# Details
The contests asks for an implementation of `match` that is normal correct, but returns false positives when a certain set of conditions are met. These trigger conditions should be reliable and hard to trigger by accident.

Part one of `main.h` demonstrates how `match` returns correct (non-underhanded) results. Pretty boring. So let's look at how the underhanded behavior is triggered. Feel free to skip to the bottom for an overview of the end-to-end underhanded behavior.

## Triggering the Underhanded Behavior 
The trigger for the underhanded behavior is C's [floating point environment `<fenv.h>`][fenv]. The floating point environment error state provides a set of one bit global variables that a programmer may not be failure. Even better, harmless looking actions that don't even mention `fenv` can set these variables and this can all happen in a completely unrelated part of the code, making this a good underhanded trigger mechanism. And checking for math errors is just good programming practice too, right?

For this project, I choose to trigger the underhanded behavior using the `FE_INVALID` floating point environment error flag. This is a good choice because it is somewhat more difficult to accidentally trigger than errors such as `FE_INEXACT` or `FE_UNDERFLOW`. Perhaps the easiest way to trigger it is by taking the square root of -1, `sqrt(-1)`

```
double test[] = { 1.0, 2.0, 1.5, -3 };
double reference[] = { 0.4, 2.2, 0.9, -2.8 };

// Correct behavior
assert(match(test, reference, 4, 0) == 0);

// Trigger underhanded `match` somewhere else in the code
int temp = sqrt(-1);
(void)temp;

// The arrays always match now. Amazing!
assert(match(test, reference, 4, 0) == 1);
```

### Underhanded Error Handling
But let's look at the code that gets triggered using the above. The comparison algoritm I use is extremely basic, it just checks the difference between the values at every index in the array. If this difference is ever greater than `threshold`, the arrays do not match. Otherwise they do.

Here's the function that compare individual elements at a given index. 

```
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

Seems simple. Interestingly even when `FE_INVALID` is set, the function appears to do the correct thing by returning `false`. But it also calls the error handler function `on_error` when a math error occurs. How nice of the programmer. Still, it's not like we use the result of the error handler or use something like `setjmp` anywhere in the code. What could possibly go wrong...

### Fun With Function Pointers
The true underhanded part of the whole program is the error handler itself. `error_handler` is a function pointer. But one interesting feature of function pointers is that you cast any data to a function pointer, and most compilers will happily still call the result and treat the data as machine code.

```
typedef void(*error_handler)();

unsigned char data[] = { 0xC3 /* ret in x86 assembly */ };
printf("%d", ((error_handler)data)());
```

Even better, you can cast a `char*` string to a function pointer. This basically allows us to do whatever the hell we want to the program by constructing very specific strings of data that represent interesting machine code. In the case of the above, all we really want to do is bypass the `do_elements_match` check whenever `FE_INVALID` and always return true. This can be accomplished by jumping two levels up the stack: first out of called function and then out of `do_elements_match`. Here's some assembly that does just that:

```
mov esp, ebp
pop ebp
ret 0
```

In x86 machine code, this is `89 ec 5d c2 00 00` which conveniently looks a lot like a unicode string.

### A Most Evil Error Handler 
Localization is important for any program, so it makes sense to localize any output messages so that people in the Glorious Democratic Peoples Republic Of Bob can understand them. Fortunately for us, the word `Error` in bobspeak just happens to be `觬巂`. And in unicode, this is: `\x89\xec\x5d\xc2\0\0` (extra zeros are required at the end)

```
#define error_label "\x89\xec\x5d\xc2\0\0:"

static void* error_message = error_label " - A math error occured. Returning no match found.";
```

`error_message` is just a string but if we look at the binary data of said string, you would notice that it starts with the same values as the machine code example up above.

At this point, the programmer implemented a default error handler to print our this error message:

```
void error_messager() {
    printf("%s", error_message);
}
```

But what a poor choice of names: `error_messager` and `error_message`! Who would notice if you accidentally swapped one in for the other? It probably was just a silly typo too. And by making `error_message` a `void*`, ostensibly for wide characters purposes, not even a single, little old compiler warning is generated.

```
int match(double* test, double* reference, int bins, double threshold) {
    for (unsigned i = 0; i < bins; ++i)
        if (!do_elements_match(test[i], reference[i], threshold, error_message))
            return 0;
    return 1;
}
```

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





[fenv]: http://en.cppreference.com/w/c/numeric/fenv