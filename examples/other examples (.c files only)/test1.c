/*
 * test_errors.c
 * This file contains invalid syntax to test the scanner's error handling.
 */

int main() {
    int valid_variable = 10;

    // First error: An invalid character '$' used in an expression.
    // The scanner should stop here.
    int invalid_sum = valid_variable + 50;

    // The code below should not be scanned if the program halts on the first error.
    
    // Second error type: A number with multiple decimal points.
    float bad_pi = 3.14159; 

    return 0;
}

// An unterminated block comment to see how the scanner handles it.
/* This comment never ends.*/