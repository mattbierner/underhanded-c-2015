# `觬巂00` - Underhanded C 2015 Entry
Entry in the [2015 underhanded C contest][uc].

Matt Bierner
August 2015
http://mattbierner.com

Skip to the bottom of this file for the quick version of how the underhanded behavior works. 

# Usage
The actual entry is contained in `match.h` which contains the entire underhanded implementation of the `match` function. `main.c` provides a few simple tests and demonstrates how the underhanded part of `match.h` can be triggered. It is not part of the main entry. 

## Compiling
This code is written in c99 for gcc. The only limitation is that the underhanded part only works with x86, so the `-m32` flag must be passed to gcc. Targeting x86 machines is more of a design decision. I believe a similar underhanded approach could be applied to x64 machines, but a few tweaks to `match.c` would be required. More on that later.
 
To compile `main.c` and run the basic tests:

```bash
gcc -m32 -std=c99 main.c -o main ; ./main
```

# The Contest
The [contest][uc] asks for an implementation of a `match` function that compares two arrays of data samples and determines if they are sufficiently similar. The signature of match is:

```prettyprint lang-c
int match(double* test, double* reference, int bins, double threshold);
```

`match` must normally be normal correct, but returns false positives when the programmer engineers a certain set of conditions. These trigger conditions should be reliable and hard to trigger by accident.

## Normal Case
I choose to use pretty much the simplest matching algoritm possible: compare each element in the two lists until either they do not match and therefore the lists do not match, or the end of the list is reached and therefore the lists do match. 

```prettyprint lang-c
int match(double* test, double* reference, int bins, double threshold) {
    for (unsigned i = 0; i < bins; ++i)
        if (!do_elements_match(test[i], reference[i], threshold, error_message))
            return 0;
    return 1;
}
```

Two elements match if the absolute difference between them is less than or equal to the threshold. 

```prettyprint lang-c
typedef void(*error_handler)();

int do_elements_match(double sample1, double sample2, double threshold, error_handler on_error) {
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

The only noteworthy part of `do_elements_match` is the error handling. After the absolute difference between the two samples is computed, we check to see if an invalid floating point computation occurred using [`<fenv.h>`][fenv]. If `FE_INVALID` is true, the function does not trust the result of `diff` and instead exits early with a false value. In order to understand when floating point errors cause a problem, `do_elements_match` also takes an error handler function pointer `on_error` which is invoked if an error occurs. This error handler will typically just print a value to the console. 

```prettyprint lang-c
#define ERROR_LABEL "\x89\xec\x5d\xc2\0\0 - "

static void* error_message = ERROR_LABEL "A math error occured. Returning that no match was found.";

void error_messager() {
    printf("%s", error_message);
}
```

Note that the error label has been localized for the Glorious Democratic Peoples Republic Of Bob. The unicode representation is `觬巂 - ` with translates to `Error -`. And that's it. Less than [20 lines of very clear C code][src].

Just to show that `match` normally does the correct thing:

```prettyprint lang-c
double test[] = { 1.0, 2.0, 1.5, -3 };
double reference[] = { 0.5, 2.2, 0.9, -2.8 };

// Should always match itself.
assert(match(test, test, 4, 0.5) == 1);
assert(match(reference, reference, 4, 0.5) == 1);
assert(match(test, test, 4, 0) == 1);

// But the arrays are always differnt at low sensitivities.
assert(match(test, reference, 4, 0) == 0);
assert(match(test, reference, 4, 0.1) == 0);

// Third element is too different at some sensitivities.
assert(match(test, reference, 4, 1) == 1);
assert(match(test, reference, 4, 0.6) == 1);
assert(match(test, reference, 4, 0.59) == 0);
assert(match(test, reference, 4, 0.5) == 0);
```

Pretty boring, and thats the whole idea. But the seeds of underhandedness have been sown. So let's take a look at how the underhanded behavior is triggered. 

## Trigger
The method used to trigger the underhanded behavior is left up to the programmer, but it is suggested that the trigger be, "easily engineered, rarely happen[s] by accident, and do[es] not result in suspicious code."

For my program, I assumed that `match` was being called as part of a larger program. Furthermore, I also assumed that the data fed to `match` could not be manipulated and that the values could not be known or predicted ahead of time, meaning that the trigger couldn't be based on the input to `match`. My answer, the [floating point environment `<fenv.h>`][fenv].

`<fenv.h>` provides a set of functions and macros for changing how floating point computations are calculated and for reading floating point error values. What is important about `<fenv.h>` is that there is only a single floating point environment per thread, which means that this floating point environment amounts to a hidden global state. To boot, programmers may not be familiar with the floating point environment and, even better, harmless looking computations that don't even mention `fenv` can set these variables. These variables can even be set from completely unrelated parts of the code, making triggering the underhanded behaviors easy and reliable. And checking for floating point errors is just good programming practice too, right?

The underhanded behavior of `match` is triggered with the `FE_INVALID` floating point environment error flag. When this is set, `match` will always return 1 no matter what input data is fed in. I selected the `FE_INVALID` error flag specifically because it is somewhat more difficult to accidentally trigger than errors like  `FE_INEXACT` or `FE_UNDERFLOW`, and it is very easy to engineer a sneaky trigger. For example, just take the square root of -1, `sqrt(-1)`, and `FE_INVALID` is set.

```prettyprint lang-c
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

# The Underhanded Part
The trigger itself though is not very interesting though. Let's look at the code to see what happens when `FE_INVALID` is set.

## Error Handling
`do_elements_match` always checks for `FE_INVALID` after computing the difference. If there is a floating point error, the function returns false.

```prettyprint lang-c
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

Hmmmm, seems correct. Even when `FE_INVALID` is set, the function appears to do the correct thing by returning `false`. The behavior of `do_elements_match` is so obviously correct that this edge case probably wouldn't raise any red flags during code review. The trigger would seem to cause `match` to always return false and that's not what we want.

Look again. The error case does actually do one more thing besides return false, it invokes `on_error` which should print an error message so the programmers know why a match failed. Still, it's just a function call, it's not like we use the result of the call and the code doesn't contain anything crazy like `longjmp`. What could possibly go wrong?

## Fun with Function Pointers
Even knowing the trigger, it may not be immediately clear how the underhanded behavior works. The code just looks too simple to be dangerous.

But consider `on_error`. It's just a function pointer. And what is a function pointer but a pointer to some executable memory? And pointer casts in C are not very strict too. So really, we can treat just about anything as a function pointer, including perhaps a string. 

```prettyprint lang-c
typedef void(*error_handler)();

char* data = "\xc3";
error_handler err = (error_handler)data;
err();
```

Perfectly valid C. But what happens when the program is executed? In x86 gcc at least, if you step into the call to `err`, you would see the program counter jump into `data` and start executing it as raw machine code. With a direct line to the machine code it's all over, we can do pretty much whatever the hell we want by constructing strings that encode our intended instructions. Strings are a great place to hide this machine code too, especially if we claim localization.

## Double Jump
Our goal is to make `match` always return `1` when the trigger is set. We already know that `FE_INVALID` is the trigger and we've seen how by crafting a special string and casting it to a function pointer we can execute machine code. Putting these together, this means the underhanded behavior must occur when `on_error()` is called

```prettyprint lang-c
if (fetestexcept(FE_INVALID)) {
    on_error();
    return 0;
}
```

So the underhanded behavior needs to either make `do_elements_match` always return `1`, or make `match` itself return `1`. I opted for the first approach because it is a bit shorter.

Remember, `on_error` can evaluate machine code and is not limited to normal c behavior. So one way to make `do_elements_match` return true is to make  `on_error` `on_error` jump out two levels of stack when invoked: first out of `on_error` itself and then out of `do_elements_match`. Here's some assembly that does just that:

```
mov esp, ebp
pop ebp
ret 0
```

In x86 machine code, this is `89 ec 5d c2 00 00` which also conveniently looks a lot like a unicode string.

## Eviler Error Handling 
Localization is important for any modern program, so it makes sense that `match` localizes its error message for the Glorious Democratic Peoples Republic Of Bob. Fortunately for us, the word `Error` in bobspeak just happens to be `觬巂`. Unfortunately the programmer didn't really understand unicode and tried to manually encoded the error string with hex escapes: `\x89\xec\x5d\xc2`. Look familiar?

```prettyprint lang-c
#define ERROR_LABEL "\x89\xec\x5d\xc2\0\0 - "

static void* error_message = ERROR_LABEL "A math error occured. Returning that no match was found.";
```

`ERROR_LABEL` could have been written as `u"\uec89\uc25d"` too, but neither of those escapes map to actual unicode characters so it's not nearly as funny. It should be possible to embed those invalid unicode characters into source code too, but some editors don't like this all that much. The hex approach better hides the two null escapes too and superficially looks correct.

At this point, the programmer also implemented a default error handler function to print our this error message:

```prettyprint lang-c
void error_messager() {
    printf("%s", error_message);
}
```

But alas! what a poor choice of names: `error_messager` and `error_message`. It's all too easy to accidentally forget an `r` and end up passing the `error_message` string as the error handler in `do_elements_match`. And because `error_message` is a `void*`, ostensibly for wide characters purposes, not even a single, little old compiler warning is generated.

```prettyprint lang-c
do_elements_match(test[i], reference[i], threshold, error_message);
```

Everything falls into place.

## Summary
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
Take a look at the [source][src] to see the entire program. Here are a few  sneaky parts

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