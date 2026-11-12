#pragma once
// Compiled into each generated program; the resulting executable needs no SERSEG runtime file.
static constexpr const char* serseg_runtime = R"SERSEG_RUNTIME(
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
static UINT serseg_previous_console_cp;
static void serseg_restore_console_cp(void) {
    fflush(stdout);
    fflush(stderr);
    if (serseg_previous_console_cp && GetConsoleOutputCP() == CP_UTF8)
        SetConsoleOutputCP(serseg_previous_console_cp);
}
void serseg_windows_console_init(void) {
    DWORD mode;
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!output || output == INVALID_HANDLE_VALUE || !GetConsoleMode(output, &mode)) return;
    UINT previous = GetConsoleOutputCP();
    if (previous && previous != CP_UTF8 && SetConsoleOutputCP(CP_UTF8)) {
        serseg_previous_console_cp = previous;
        if (atexit(serseg_restore_console_cp) != 0) {
            SetConsoleOutputCP(previous);
            serseg_previous_console_cp = 0;
        }
    }
}
#endif

typedef struct SersegText {
    uint64_t references;
    uint64_t length;
    char bytes[];
} SersegText;
_Static_assert(offsetof(SersegText, bytes) == 16, "unexpected text layout");

static void serseg_text_fatal(const char *message) {
    fprintf(stderr, "SERSEG runtime error: %s\n", message);
    exit(1);
}
SersegText *serseg_text_retain(SersegText *value) {
    if (!value) serseg_text_fatal("invalid text reference");
    if (value->references != UINT64_MAX) {
        if (value->references == 0 || value->references == UINT64_MAX - 1)
            serseg_text_fatal("text reference count overflow or invalid reference");
        ++value->references;
    }
    return value;
}
void serseg_text_release(SersegText *value) {
    if (!value || value->references == UINT64_MAX) return;
    if (value->references == 0) serseg_text_fatal("invalid text reference count");
    if (--value->references == 0) free(value);
}
// Consumes the two owned arguments and returns one owned result.
SersegText *serseg_text_concat(SersegText *left, SersegText *right) {
    if (!left || !right) serseg_text_fatal("invalid text reference");
    if (left->length > SIZE_MAX - sizeof(SersegText) - 1 ||
        right->length > SIZE_MAX - sizeof(SersegText) - 1 - left->length)
        serseg_text_fatal("text length overflow");
    size_t n = (size_t)left->length + (size_t)right->length;
    SersegText *result = (SersegText *)malloc(sizeof(SersegText) + n + 1);
    if (!result) serseg_text_fatal("text allocation failed");
    result->references = 1;
    result->length = n;
    memcpy(result->bytes, left->bytes, (size_t)left->length);
    memcpy(result->bytes + left->length, right->bytes, (size_t)right->length);
    result->bytes[n] = '\0';
    serseg_text_release(left);
    serseg_text_release(right);
    return result;
}
int64_t serseg_text_length(SersegText *value) {
    if (!value || value->length > INT64_MAX) serseg_text_fatal("text length overflow");
    int64_t length = (int64_t)value->length;
    serseg_text_release(value);
    return length;
}
// Reads one line as bytes. Newline and an immediately preceding CR are removed.
SersegText *serseg_input(void) {
    size_t capacity = 128, length = 0;
    SersegText *value = (SersegText *)malloc(sizeof(SersegText) + capacity + 1);
    if (!value) serseg_text_fatal("text allocation failed");
    int c, newline = 0;
    while ((c = fgetc(stdin)) != EOF) {
        if (c == '\n') { newline = 1; break; }
        if (c == 0) { free(value); serseg_text_fatal("input contains NUL byte"); }
        if (length == capacity) {
            size_t max = SIZE_MAX - sizeof(SersegText) - 1;
            if (capacity > max / 2) { free(value); serseg_text_fatal("input length overflow"); }
            capacity *= 2;
            SersegText *grown = (SersegText *)realloc(value, sizeof(SersegText) + capacity + 1);
            if (!grown) { free(value); serseg_text_fatal("text allocation failed"); }
            value = grown;
        }
        value->bytes[length++] = (char)(unsigned char)c;
    }
    if (ferror(stdin)) { free(value); serseg_text_fatal("input read failed"); }
    if (newline && length && value->bytes[length - 1] == '\r') --length;
    value->references = 1;
    value->length = length;
    value->bytes[length] = '\0';
    return value;
}
// All text arguments here are owned; functions consume them.
SersegText *serseg_input_prompt(SersegText *prompt) {
    if (!prompt) serseg_text_fatal("invalid text reference");
    if (fwrite(prompt->bytes, 1, (size_t)prompt->length, stdout) != prompt->length || fflush(stdout) != 0) {
        serseg_text_release(prompt);
        serseg_text_fatal("prompt output failed");
    }
    serseg_text_release(prompt);
    return serseg_input();
}
static SersegText *serseg_text_copy(const char *bytes, size_t length) {
    if (length > SIZE_MAX - sizeof(SersegText) - 1 || length > INT64_MAX)
        serseg_text_fatal("text length overflow");
    SersegText *text = (SersegText *)malloc(sizeof(SersegText) + length + 1);
    if (!text) serseg_text_fatal("text allocation failed");
    text->references = 1;
    text->length = length;
    memcpy(text->bytes, bytes, length);
    text->bytes[length] = '\0';
    return text;
}
SersegText *serseg_str_int(int64_t value) {
    char buffer[32];
    int n = snprintf(buffer, sizeof(buffer), "%" PRId64, value);
    if (n < 0 || (size_t)n >= sizeof(buffer)) serseg_text_fatal("integer formatting failed");
    return serseg_text_copy(buffer, (size_t)n);
}
// Accept float bits in an integer argument so both Win64 and SysV use one ABI path.
SersegText *serseg_str_float_bits(uint64_t bits) {
    double value;
    memcpy(&value, &bits, sizeof(value));
    char buffer[128];
    int n = snprintf(buffer, sizeof(buffer), "%.17g", value);
    if (n < 0 || (size_t)n >= sizeof(buffer)) serseg_text_fatal("float formatting failed");
    return serseg_text_copy(buffer, (size_t)n);
}
SersegText *serseg_str_bool(int64_t value) {
    return value ? serseg_text_copy("true", 4) : serseg_text_copy("false", 5);
}
// Return 1 for a valid value, 0 for syntax, -1 for range.
static int serseg_parse_int(const SersegText *text, int64_t *result) {
    size_t i = 0, n = (size_t)text->length;
    int negative = 0;
    if (i < n && (text->bytes[i] == '-' || text->bytes[i] == '+')) negative = text->bytes[i++] == '-';
    if (i == n) return 0;
    uint64_t limit = negative ? (uint64_t)INT64_MAX + 1u : (uint64_t)INT64_MAX;
    uint64_t value = 0;
    for (; i < n; ++i) {
        unsigned char ch = (unsigned char)text->bytes[i];
        if (ch < '0' || ch > '9') return 0;
        unsigned digit = (unsigned)(ch - '0');
        if (value > (limit - digit) / 10u) return -1;
        value = value * 10u + digit;
    }
    *result = negative && value == (uint64_t)INT64_MAX + 1u ? INT64_MIN : negative ? -(int64_t)value : (int64_t)value;
    return 1;
}
int64_t serseg_text_is_int(SersegText *text) {
    if (!text) serseg_text_fatal("invalid text reference");
    int64_t value; int ok = serseg_parse_int(text, &value);
    serseg_text_release(text); return ok == 1;
}
int64_t serseg_text_to_int(SersegText *text) {
    if (!text) serseg_text_fatal("invalid text reference");
    int64_t value; int ok = serseg_parse_int(text, &value);
    serseg_text_release(text);
    if (ok < 0) serseg_text_fatal("to_int: integer out of range");
    if (!ok) serseg_text_fatal("to_int: expected signed decimal integer");
    return value;
}
// Returns binary64 bits in an integer register on both supported ABIs.
static int serseg_parse_float(const SersegText *text, uint64_t *result) {
    size_t i = 0, n = (size_t)text->length, digits = 0;
    if (i < n && (text->bytes[i] == '+' || text->bytes[i] == '-')) ++i;
    while (i < n && text->bytes[i] >= '0' && text->bytes[i] <= '9') { ++i; ++digits; }
    int valid = digits != 0;
    if (i < n && text->bytes[i] == '.') {
        ++i; digits = 0;
        while (i < n && text->bytes[i] >= '0' && text->bytes[i] <= '9') { ++i; ++digits; }
        if (!digits) valid = 0;
    }
    if (i < n && (text->bytes[i] == 'e' || text->bytes[i] == 'E')) {
        ++i; if (i < n && (text->bytes[i] == '+' || text->bytes[i] == '-')) ++i;
        digits = 0;
        while (i < n && text->bytes[i] >= '0' && text->bytes[i] <= '9') { ++i; ++digits; }
        if (!digits) valid = 0;
    }
    if (!valid || i != n) return 0;
    errno = 0;
    char *end = NULL;
    double value = strtod(text->bytes, &end);
    if (errno == ERANGE || !isfinite(value)) return -1;
    if (end != text->bytes + n) return 0;
    memcpy(result, &value, sizeof(value));
    return 1;
}
int64_t serseg_text_is_float(SersegText *text) {
    if (!text) serseg_text_fatal("invalid text reference");
    uint64_t bits; int ok = serseg_parse_float(text, &bits);
    serseg_text_release(text); return ok == 1;
}
uint64_t serseg_text_to_float_bits(SersegText *text) {
    if (!text) serseg_text_fatal("invalid text reference");
    uint64_t bits; int ok = serseg_parse_float(text, &bits);
    serseg_text_release(text);
    if (ok < 0) serseg_text_fatal("to_float: number out of range");
    if (!ok) serseg_text_fatal("to_float: expected decimal number");
    return bits;
}
static int serseg_ascii_space(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}
SersegText *serseg_text_trim(SersegText *text) {
    if (!text) serseg_text_fatal("invalid text reference");
    size_t a = 0, b = (size_t)text->length;
    while (a < b && serseg_ascii_space((unsigned char)text->bytes[a])) ++a;
    while (b > a && serseg_ascii_space((unsigned char)text->bytes[b-1])) --b;
    SersegText *result = serseg_text_copy(text->bytes + a, b - a);
    serseg_text_release(text);
    return result;
}
SersegText *serseg_text_slice(SersegText *text, int64_t start, int64_t end) {
    if (!text) serseg_text_fatal("invalid text reference");
    if (start < 0 || end < start || (uint64_t)end > text->length) {
        serseg_text_release(text); serseg_text_fatal("slice: byte range out of bounds");
    }
    SersegText *result = serseg_text_copy(text->bytes + (size_t)start, (size_t)(end - start));
    serseg_text_release(text);
    return result;
}
// Validate UTF-8 and return the byte offset immediately following one scalar.
static size_t serseg_utf8_next(const SersegText *text, size_t i) {
    size_t n = (size_t)text->length;
    unsigned char c = (unsigned char)text->bytes[i];
    size_t count;
    if (c <= 0x7f) return i + 1;
    if (c >= 0xc2 && c <= 0xdf) count = 2;
    else if (c >= 0xe0 && c <= 0xef) count = 3;
    else if (c >= 0xf0 && c <= 0xf4) count = 4;
    else return SIZE_MAX;
    if (count > n - i) return SIZE_MAX;
    unsigned char b = (unsigned char)text->bytes[i + 1];
    if (b < 0x80 || b > 0xbf || (c == 0xe0 && b < 0xa0) ||
        (c == 0xed && b > 0x9f) || (c == 0xf0 && b < 0x90) || (c == 0xf4 && b > 0x8f)) return SIZE_MAX;
    for (size_t k = 2; k < count; ++k) {
        unsigned char next = (unsigned char)text->bytes[i + k];
        if (next < 0x80 || next > 0xbf) return SIZE_MAX;
    }
    return i + count;
}
int64_t serseg_text_utf8_length(SersegText *text) {
    if (!text) serseg_text_fatal("invalid text reference");
    size_t i = 0; int64_t count = 0;
    while (i < text->length) {
        size_t next = serseg_utf8_next(text, i);
        if (next == SIZE_MAX) { serseg_text_release(text); serseg_text_fatal("utf8_len: invalid UTF-8"); }
        if (count == INT64_MAX) { serseg_text_release(text); serseg_text_fatal("utf8_len: length overflow"); }
        ++count; i = next;
    }
    serseg_text_release(text); return count;
}
SersegText *serseg_text_utf8_slice(SersegText *text, int64_t start, int64_t end) {
    if (!text) serseg_text_fatal("invalid text reference");
    if (start < 0 || end < start) { serseg_text_release(text); serseg_text_fatal("utf8_slice: range out of bounds"); }
    size_t i = 0, a = SIZE_MAX, b = SIZE_MAX; int64_t count = 0;
    while (1) {
        if (count == start) a = i;
        if (count == end) b = i;
        if (i == text->length) break;
        size_t next = serseg_utf8_next(text, i);
        if (next == SIZE_MAX) { serseg_text_release(text); serseg_text_fatal("utf8_slice: invalid UTF-8"); }
        if (count == INT64_MAX) { serseg_text_release(text); serseg_text_fatal("utf8_slice: length overflow"); }
        ++count; i = next;
    }
    if (a == SIZE_MAX || b == SIZE_MAX) { serseg_text_release(text); serseg_text_fatal("utf8_slice: range out of bounds"); }
    SersegText *result = serseg_text_copy(text->bytes + a, b - a);
    serseg_text_release(text); return result;
}
int64_t serseg_text_starts_with(SersegText *text, SersegText *prefix) {
    if (!text || !prefix) serseg_text_fatal("invalid text reference");
    int64_t found = prefix->length <= text->length && memcmp(text->bytes, prefix->bytes, (size_t)prefix->length) == 0;
    serseg_text_release(text); serseg_text_release(prefix); return found;
}
int64_t serseg_text_ends_with(SersegText *text, SersegText *suffix) {
    if (!text || !suffix) serseg_text_fatal("invalid text reference");
    int64_t found = suffix->length <= text->length && memcmp(text->bytes + (size_t)(text->length - suffix->length), suffix->bytes, (size_t)suffix->length) == 0;
    serseg_text_release(text); serseg_text_release(suffix); return found;
}
int64_t serseg_text_contains(SersegText *text, SersegText *part) {
    if (!text || !part) serseg_text_fatal("invalid text reference");
    int64_t found = 0;
    if (part->length <= text->length) {
        size_t limit = (size_t)(text->length - part->length);
        for (size_t i = 0; i <= limit; ++i) {
            if (memcmp(text->bytes + i, part->bytes, (size_t)part->length) == 0) { found = 1; break; }
        }
    }
    serseg_text_release(text); serseg_text_release(part); return found;
}
int64_t serseg_text_equal(SersegText *left, SersegText *right) {
    if (!left || !right) serseg_text_fatal("invalid text reference");
    int64_t equal = left->length == right->length &&
        memcmp(left->bytes, right->bytes, (size_t)left->length) == 0;
    serseg_text_release(left);
    serseg_text_release(right);
    return equal;
}

// Arrays store primitive values or text references; nested arrays and cycles are not supported.
typedef struct SersegArray { uint64_t references; uint64_t length; uint64_t capacity; uint64_t is_text; uint64_t *items; } SersegArray;
SersegArray *serseg_array_new(uint64_t count, uint64_t is_text) {
    if (count > INT64_MAX || count > SIZE_MAX / sizeof(uint64_t))
        serseg_text_fatal("array size overflow");
    SersegArray *array = (SersegArray *)malloc(sizeof(SersegArray));
    if (!array) serseg_text_fatal("array allocation failed");
    array->items = count ? (uint64_t *)calloc((size_t)count, sizeof(uint64_t)) : NULL;
    if (count && !array->items) { free(array); serseg_text_fatal("array allocation failed"); }
    array->references = 1;
    array->is_text = is_text;
    array->length = count;
    array->capacity = count;
    return array;
}
SersegArray *serseg_array_retain(SersegArray *array) {
    if (!array || array->references == 0 || array->references == UINT64_MAX)
        serseg_text_fatal("invalid array reference count");
    ++array->references;
    return array;
}
void serseg_array_release(SersegArray *array) {
    if (!array) return;
    if (array->references == 0) serseg_text_fatal("invalid array reference count");
    if (--array->references == 0) { if (array->is_text) for (uint64_t i = 0; i < array->length; ++i) serseg_text_release((SersegText *)(uintptr_t)array->items[i]); free(array->items); free(array); }
}
void serseg_array_set_borrowed(SersegArray *array, int64_t index, uint64_t value) {
    if (!array || index < 0 || (uint64_t)index >= array->length)
        serseg_text_fatal("array index out of bounds");
    if (array->is_text) serseg_text_release((SersegText *)(uintptr_t)array->items[index]);
    array->items[index] = value;
}
void serseg_array_push(SersegArray *array, uint64_t value) {
    if (!array || array->length >= INT64_MAX) serseg_text_fatal("array length overflow");
    if (array->length == array->capacity) {
        uint64_t limit = SIZE_MAX / sizeof(uint64_t);
        if (limit > INT64_MAX) limit = INT64_MAX;
        uint64_t capacity = array->capacity ? array->capacity : 4;
        if (capacity <= array->length) capacity = capacity > limit / 2 ? limit : capacity * 2;
        if (capacity <= array->length || capacity > limit) serseg_text_fatal("array size overflow");
        uint64_t *items = (uint64_t *)realloc(array->items, (size_t)capacity * sizeof(uint64_t));
        if (!items) serseg_text_fatal("array allocation failed");
        array->items = items;
        array->capacity = capacity;
    }
    array->items[array->length++] = value;
    serseg_array_release(array);
}
void serseg_array_set(SersegArray *array, int64_t index, uint64_t value) {
    serseg_array_set_borrowed(array, index, value);
    serseg_array_release(array);
}
uint64_t serseg_array_get_borrowed(SersegArray *array, int64_t index) {
    if (!array || index < 0 || (uint64_t)index >= array->length)
        serseg_text_fatal("array index out of bounds");
    uint64_t value = array->items[index];
    if (array->is_text) serseg_text_retain((SersegText *)(uintptr_t)value);
    return value;
}
uint64_t serseg_array_get(SersegArray *array, int64_t index) {
    uint64_t value = serseg_array_get_borrowed(array, index);
    serseg_array_release(array);
    return value;
}
int64_t serseg_array_length(SersegArray *array) {
    if (!array || array->length > INT64_MAX) serseg_text_fatal("array length overflow");
    int64_t length = (int64_t)array->length;
    serseg_array_release(array);
    return length;
}

)SERSEG_RUNTIME";
