/*
 * string_utils.h - String manipulation utilities for smf-spf
 *
 * This file is part of smf-spf.
 *
 * smf-spf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * smf-spf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SMF_SPF_STRING_UTILS_H
#define SMF_SPF_STRING_UTILS_H

#include <stddef.h>

/**
 * @brief Safely copy a string with size limitation
 *
 * Copies up to size-1 characters from src to dst and ensures
 * null termination. Similar to strlcpy() but more portable.
 *
 * @param dst Destination buffer
 * @param src Source string
 * @param size Size of destination buffer
 */
void strscpy(char *dst, const char *src, size_t size);

/**
 * @brief Convert string to lowercase in-place
 *
 * Converts all uppercase ASCII characters to lowercase.
 * Non-ASCII characters are left unchanged.
 *
 * @param str String to convert (modified in-place)
 */
void strtolower(char *str);

/**
 * @brief Trim leading and trailing whitespace from a string
 *
 * Removes whitespace characters from both ends of the string.
 * The string is modified in-place.
 *
 * @param str String to trim
 * @return Pointer to the trimmed string (same as input)
 */
char *trim_space(char *str);

#endif /* SMF_SPF_STRING_UTILS_H */
