/**
    Underhanded C Contest 2015
    
    August 2015
    Matt Bierner
    http://mattbierner.com
*/
#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*------------------------------------------------------------------------------
    Program
------------------------------------------------------------------------------*/
/**
    Function invoked when a floating point error occurs.
*/
typedef void(*error_handler)();

/**
    "觬巂:" - "Error:"
 
    Localized error label for the Glorious Democratic Peoples Republic Of Bob.
*/
#define error_label "\x89\xec\x5d\xc2\0\0:"

/**
    Message printed when a floating point error occurss.
    
    TODO: Localize for the GDPRB.
*/
static void* error_message = error_label " - A math error occured. Returning no match found.";

/**
    Default error message implementation.
    
    Prints the error to the console.
*/
void error_messager()
{
    printf("%s", error_message);
}

/**
    Check if the two samples match.
    
    Elements match if the difference between them is less than a given threshold.
 
    @param sample1 First sample value.
    @param sample2 Other sample value.
    @param threshold Greatest acceptable sample difference for a match.
    @param on_error Function invoked when an error occures
 
    @return Did the two samples match? Always return false if an error occured.
*/
int do_elements_match(double sample1, double sample2, double threshold, error_handler on_error)
{
    double diff = fabs(sample1 - sample2);
    // Check to see if a math error occured.
    if (fetestexcept(FE_INVALID)) {
        on_error();
        return 0;
    }
    return diff < threshold;
}

/**
    Check if two sample collections match.
    
    Two collections match if every element in the collection matches. Matching
    is conservative and treats mathmatical errors such as overflow as failures.
    
    If an error occurs, prints out an error message and return false.

*/
int match(double* test, double* reference, int bins, double threshold)
{
    for (unsigned i = 0; i < bins; ++i)
        if (!do_elements_match(test[i], reference[i], threshold, error_message))
            return 0;
    return 1;
}

/*------------------------------------------------------------------------------
    Testing
------------------------------------------------------------------------------*/
/**
    Set every element in a sample array to zero.
*/
double* null_array(double* data, unsigned count)
{
    return memset(data, 0, count * sizeof(double));
}

/**
    Add `+/-jitter_range` random variation to each element in an array.
*/
double* add_jitters(double* start, unsigned count, double jitter_range)
{
    double* i = start;
    double* end = start + count;
    while (i < end)
        (*i++) += (rand() / (double)RAND_MAX * jitter_range * 2) - jitter_range;
    return start;
}


int main(int argc, const char* argv[])
{
feclearexcept(FE_ALL_EXCEPT);
    double test[] = { 1.0, 2.0, 1.5, -3 };

    double reference[] = { 0.4, 2.2, 1.3, -2.3 };

    printf("%d\n", match(test, test, 4, 0.5));
    
    printf("%d\n", match(test, reference, 4, 0.5));

    double yyy = sqrt(-1);
    (void)yyy;
    printf("%d\n", match(test, reference, 4, 0.5));

    //feclearexcept(FE_ALL_EXCEPT);

    double dynTest[100];
    double dynRef[100];

    for (unsigned i = 0; i < 50; ++i) {
        add_jitters(null_array(dynTest, 100), 100, 1.0);
        
        memcpy(dynRef, dynTest, 100 * sizeof(double));
        
        add_jitters(dynTest, 100, 1);
        
        printf("%d\n", match(dynTest, dynRef, 100, 0.5));
    }

    
    return 0;
}
