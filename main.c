/**
    Test and example usage of the underhanded `match` found in `match.h`

    Not part of the main entry. There is no underhanded logic in this file
    and the underhanded part of `match.h` does not depend on this file in any way.
 
    August 2015
    Matt Bierner
    http://mattbierner.com
*/
#include <assert.h>
#include "match.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
    Set every element in a sample array to zero.
*/
double* null_array(double* data, unsigned count) {
    return memset(data, 0, count * sizeof(double));
}

/**
    Add `+/-jitter_range` random variation to each element in an array.
*/
double* add_jitters(double* start, unsigned count, double jitter_range) {
    double* i = start;
    double* end = start + count;
    while (i < end)
        (*i++) += (rand() / (double)RAND_MAX * jitter_range * 2) - jitter_range;
    return start;
}

/**
    Demonstrates the underhanded use of `match`. See the readme for more details
*/
int main(int argc, const char* argv[])
{
    feclearexcept(FE_ALL_EXCEPT);
    
    // Simple case, two short arrays that should not match
    double test[] = { 1.0, 2.0, 1.5, -3 };
    double reference[] = { 0.4, 2.2, 0.9, -2.8 };
    
    {
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
    }
    
    // Ranomly generate some array data that should or should not match depending
    // on the sensitivity. Still just testing the good case.
    const int testSize = 100;
    double dynTest[testSize];
    double dynRef[testSize];
    
    for (unsigned i = 0; i < 50; ++i) {
        add_jitters(null_array(dynTest, testSize), testSize , 1.0);
        memcpy(dynRef, dynTest, testSize * sizeof(double));
        add_jitters(dynTest, 100, 1);
        
        assert(match(dynTest, dynRef, testSize, 1.5) == 1);
        assert(match(dynTest, dynRef, testSize, 1) == 1);
        assert(match(dynTest, dynRef, testSize, 0.8) == 0);
        assert(match(dynTest, dynRef, testSize, 0) == 0);
    }

    // Now the underhanded part.
    // Somewhere before match is called, tigger `FE_INVALID` to be set.
    // This can happen in any other part of the program but one quick way to do
    // this is by taking the square root of -1. The triggering call can be
    // hidden very easily as well.
    int temp = sqrt(-1);
    (void)temp;
    
    // Until this flag is cleared again, all match calls return true.
    
    // Same fixed size tests as before but now everything magically matches!
    {
        assert(match(test, reference, 4, 0) == 1);
        assert(match(test, reference, 4, 0.1) == 1);
        
        assert(match(test, reference, 4, 1) == 1);
        assert(match(test, reference, 4, 0.6) == 1);
        assert(match(test, reference, 4, 0.59) == 1);
        assert(match(test, reference, 4, 0.5) == 1);
    }
    
    // And testing the dynamic ones again
    for (unsigned i = 0; i < 50; ++i) {
        add_jitters(null_array(dynTest, testSize), testSize , 1.0);
        memcpy(dynRef, dynTest, testSize * sizeof(double));
        add_jitters(dynTest, 100, 1);
        
        assert(match(dynTest, dynRef, testSize, 1.5) == 1);
        assert(match(dynTest, dynRef, testSize, 1) == 1);
        assert(match(dynTest, dynRef, testSize, 0.8) == 1);
        assert(match(dynTest, dynRef, testSize, 0) == 1);
    }
    
    // Clear the FE_INVALID flag to get the normal behavior back
    feclearexcept(FE_ALL_EXCEPT);
    
    assert(match(test, reference, 4, 0) == 0);

    return 0;
}
