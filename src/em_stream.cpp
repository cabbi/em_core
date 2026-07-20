#include "em_stream.h"

size_t EmPrint::printf(const char *format, ...) {
    // Set up a local stack buffer for the formatted string
    char buffer[256]; 

    // Initialize the variable argument list
    va_list args;
    va_start(args, format);

    // Format the string safely. vsnprintf returns the total characters needed.
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Handle edge-cases and output the results
    if (len < 0) {
        return 0; // Formatting failed
    }
    return print(buffer);
}
