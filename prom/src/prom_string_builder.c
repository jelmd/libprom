/**
 * Copyright 2019-2020 DigitalOcean Inc.
 * Copyright 2021 Jens Elkner <jel+libprom@cs.uni-magdeburg.de>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stddef.h>
#include <stdint.h>

// Public
#include "prom_alloc.h"

// Private
#include "prom_assert.h"
#include "prom_string_builder.h"

// The initial capacity of the string builder.
#define PROM_STRING_BUILDER_INIT_SIZE 128

// prom_string_builder_init prototype declaration
static int psb_init(psb_t *self);

struct psb {
	char *str;			/**< the target string  */
	size_t allocated;	/**< the size allocated to the string in bytes */
	size_t len;			/**< the length of str */
	size_t init_size;	/**< the initialize size of space to allocate */
};

psb_t *
psb_new(void) {
	psb_t *self = (psb_t *) prom_malloc(sizeof(psb_t));
	if (self == NULL)
		return NULL;
	self->init_size = PROM_STRING_BUILDER_INIT_SIZE;
	if (psb_init(self)) {
		psb_destroy(self);
		return NULL;
	}
	return self;
}

static int
psb_init(psb_t *self) {
	PROM_ASSERT(self != NULL);
	self->str = (char *) prom_malloc(self->init_size);
	if (self->str == NULL)
		return 1;
	*self->str = '\0';
	self->allocated = self->init_size;
	self->len = 0;
	return 0;
}

int
psb_clear(psb_t *self) {
	PROM_ASSERT(self != NULL);
	prom_free(self->str);
	self->str = NULL;
	return psb_init(self);
}

int
psb_truncate(psb_t *self, size_t len) {
	PROM_ASSERT(self != NULL);
	if (len >= self->len)
		return 0;

	self->len = len;
	self->str[self->len] = '\0';
	return 0;
}

int
psb_destroy(psb_t *self) {
	if (self == NULL)
		return 0;
	prom_free(self->str);
	self->str = NULL;
	prom_free(self);
	return 0;
}

/**
 * @brief PRIVATE Grows the size of the string given the val we want to add
 *
 * The method continuously shifts left until the new size is large enough to
 * accommodate add_len. This private method is called in methods that need to
 * add one or more characters to the underlying string.
 */
static int
psb_ensure_space(psb_t *self, size_t add_len) {
	PROM_ASSERT(self != NULL);
	size_t required = self->len + add_len + 1;
	if (add_len == 0 || self->allocated >= required)
		return 0;
    if (add_len > SIZE_MAX - self->len - 1)
        return 1;
	size_t sz = self->allocated;
	while (sz < required) {
        if (sz > SIZE_MAX / 2)
            return 1;
		sz <<= 1;
    }
	char *str = (char *) prom_realloc(self->str, sz);
	if (str == NULL)
		return 1;
	self->str = str;
    self->allocated = sz;
	return 0;
}

int
psb_add_str(psb_t *self, const char *str) {
	PROM_ASSERT(self != NULL);
	if (str == NULL || *str == '\0')
		return 0;

	size_t len = strlen(str);
	if (psb_ensure_space(self, len))
		return 1;

	memcpy(self->str + self->len, str, len);
	self->len += len;
	self->str[self->len] = '\0';
	return 0;
}

int
psb_add_char(psb_t *self, char c) {
	PROM_ASSERT(self != NULL);
	if (psb_ensure_space(self, 1))
		return 1;

	self->str[self->len] = c;
	self->len++;
	self->str[self->len] = '\0';
	return 0;
}

size_t
psb_len(psb_t *self) {
	PROM_ASSERT(self != NULL);
	return self->len;
}

char *
psb_dump(psb_t *self) {
	PROM_ASSERT(self != NULL);
	// +1 to accommodate \0
	char *out = (char *) prom_malloc((self->len + 1) * sizeof(char));
	memcpy(out, self->str, self->len + 1);
	return out;
}

char *
psb_str(psb_t *self) {
	PROM_ASSERT(self != NULL);
	return self->str;
}
