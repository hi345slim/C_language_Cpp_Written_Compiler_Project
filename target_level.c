/* target_level.c
   Brief description of the C language level this compiler is targeting.
   Edit this file to document the exact C features and standard you'll support
   (e.g., "C89 subset", "C99 without variable length arrays", "C11 (no threads)",
   etc.)

   Example:
   Target: subset of C99 — integer/floating/string/char literals, basic arithmetic,
   function definitions, declarations, preprocessor directives, comments.
*/

/* Target: subset of C99 (basic features) */

#include <stdio.h>
int main() {
    int x,y;
    // This is a single-line comment
    if (x == 42) {
        /* This is
           a block
           comment */
        x = x-3;
        int _xyzw;
    } else {
        y = 3.1; // Another comment
    }
    return 0;
}