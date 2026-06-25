#include "em_string.h"

bool toInt(const char* str, int32_t& value, bool strictParse) {
    // NOTES: 
    //  - No 'atoi' (unsafe, no error reporting)
    //  - No 'strtol' (heavy on embedded, locale‑dependent)
    if (!str) {
        return false;
    }

    // Skip leading whitespace
    while (*str == ' ' || *str == '\t') str++;

    // Optional sign
    bool negative = false;
    if (*str == '-') {
        negative = true;
        str++;
    } else if (*str == '+') {
        str++;
    }

    // Must have at least one digit
    if (!isdigit((unsigned char)*str)) return false;

    int32_t result = 0;

    while (*str && isdigit((unsigned char)*str)) {
        int digit = *str - '0';

        // Overflow check
        if (result > (INT32_MAX - digit) / 10) return false;

        result = result * 10 + digit;
        str++;
    }

    // STRICT: no trailing characters allowed
    if (*str != '\0') return false;

    value = negative ? -result : result;
    return true;
}
