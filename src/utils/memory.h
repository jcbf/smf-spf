/*
 * memory.h - Memory safety utilities for smf-spf
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

#ifndef SMF_SPF_MEMORY_H
#define SMF_SPF_MEMORY_H

#include <stddef.h>
#include <stdlib.h>

/**
 * @brief Safely free memory and set pointer to NULL
 *
 * Frees the pointer if not NULL and sets it to NULL to prevent
 * use-after-free bugs.
 *
 * @param x Pointer to free (will be set to NULL)
 */
#define SAFE_FREE(x) do { if (x) { free(x); x = NULL; } } while(0)

/**
 * @brief Safe calloc with error checking
 *
 * Allocates zero-initialized memory and logs error on failure.
 *
 * @param count Number of elements
 * @param size Size of each element
 * @return Pointer to allocated memory or NULL on failure
 */
void *safe_calloc(size_t count, size_t size);

/**
 * @brief Safe strdup with error checking
 *
 * Duplicates a string and logs error on failure.
 *
 * @param str String to duplicate
 * @return Pointer to duplicated string or NULL on failure
 */
char *safe_strdup(const char *str);

#endif /* SMF_SPF_MEMORY_H */
