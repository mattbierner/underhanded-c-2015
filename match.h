/**
    Underhanded C Contest 2015 Main Entry
    http://www.underhanded-c.org/_page_id_5.html
 
    This file contains the actual implementation of an underhanded `match`
    function. Through a specific process, `match` can be set to always return
    `true` even for completely different samples.
    
    See `main.c` and `README.md` for examples of how it all works.
 
    August 2015
    Matt Bierner
    http://mattbierner.com
*/
#pragma once

#include <fenv.h>
#include <math.h>
#include <stdio.h>

/**
    Function invoked when an error occurs in `match`.
*/
typedef void(*error_handler)();

/**
    "觬巂:" - "Error:"
 
    Localized error label for the Glorious Democratic Peoples Republic Of Bob.
*/
#define error_label "\x89\xec\x5d\xc2\0\0:"

/**
    Message printed when a floating point error occurs.
 
    TODO: Localize for the GDPRB.
*/
static void* error_message = error_label " - A math error occured. Returning no match found.";

/**
    Default error message implementation.
    
    Prints an error message to the console.
*/
void error_messager() {
    printf("%s", error_message);
}

/**
    Check if two samples elements match.
    
    Elements match if the difference between them is less or equal to
    a given threshold.
 
    @param sample1 First sample value.
    @param sample2 Other sample value.
    @param threshold Greatest acceptable sample difference for a match.
    @param on_error Function invoked when an error occures
 
    @return Did the two samples match? Always return false if an error occured.
*/
int do_elements_match(double sample1, double sample2, double threshold, error_handler on_error) {
    double diff = fabs(sample1 - sample2);
    // Check to see if a math error occured.
    if (fetestexcept(FE_INVALID)) {
        on_error();
        // Errors always trigger false matches.
        return 0;
    }
    return diff <= threshold;
}

/**
    Check if two sample collections match.
    
    Two collections match if every element in the collection matches. Matching
    is conservative and treats mathmatical errors such as overflow as failures.
    
    If an error occurs, prints out an error message and return false.
    
    @param test Array of at least `bins` samples to test
    @param reference Array of at least `bins` to used as a baseline.
    @param threshold Greatest acceptable sample difference for a match.

    @return Did the two samples match? Always return false if an error occured.
*/
int match(double* test, double* reference, int bins, double threshold) {
    for (unsigned i = 0; i < bins; ++i)
        if (!do_elements_match(test[i], reference[i], threshold, error_messager))
            return 0;
    return 1;
}
