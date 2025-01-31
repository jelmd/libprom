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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

// Public
#include "prom_alloc.h"
#include "prom_histogram_buckets.h"

// Private
#include "prom_assert.h"
#include "prom_log.h"

phb_t *prom_histogram_default_buckets = NULL;

static char *
double_to_str(double value) {
	char buf[32];
	int len = sprintf(buf, "%.17g", value);
	if (!strchr(buf, '.') && len < 30) {
		buf[len] = '.'; buf[len+1] = '0'; len += 2; buf[len] = '\0';
	}
	return strdup(buf);
}

phb_t *
phb_new(size_t count, double bucket, ...) {
	phb_t *self = (phb_t *) prom_malloc(sizeof(phb_t));
	if (self == NULL)
		return NULL;

	self->count = count;
	self->upper_bound = NULL;
	self->key = NULL;
	double *upper_bounds = (double *) prom_malloc(sizeof(double) * count);
	const char **keys = (const char **) prom_malloc(sizeof(char *) * count);
	if (upper_bounds == NULL || keys == NULL) {
		phb_destroy(self);
		return NULL;
	}
	upper_bounds[0] = bucket;
	keys[0] = double_to_str(bucket);
	if (count == 1) {
		self->upper_bound = upper_bounds;
		self->key = keys;
		return self;
	}
	va_list arg_list;
	va_start(arg_list, bucket);
	for (int i = 1; i < count; i++) {
		upper_bounds[i] = va_arg(arg_list, double);
		keys[i] = double_to_str(upper_bounds[i]);
	}
	va_end(arg_list);
	self->upper_bound = upper_bounds;
	self->key = keys;
	return self;
}

phb_t *
phb_linear(double start, double width, size_t count) {
	if (count <= 1) {
		PROM_WARN("count must be greater than %d", 1);
		return NULL;
	}

	phb_t *self = (phb_t *) prom_malloc(sizeof(phb_t));
	if (self == NULL)
		return NULL;
	self->upper_bound = NULL;
	self->key = NULL;
	double *upper_bounds = (double *) prom_malloc(sizeof(double) * count);
	const char **keys = (const char **) prom_malloc(sizeof(char *) * count);
	if (upper_bounds == NULL || keys == NULL) {
		phb_destroy(self);
		return NULL;
	}
	upper_bounds[0] = start;
	keys[0] = double_to_str(start);
	for (size_t i = 1; i < count; i++) {
		upper_bounds[i] = upper_bounds[i - 1] + width;
		keys[i] = double_to_str(upper_bounds[i]);
	}
	self->upper_bound = upper_bounds;
	self->key = keys;
	self->count = count;
	return self;
}

phb_t *
phb_exponential(double start, double factor, size_t count) {
	if (count < 1) {
		PROM_WARN("count must be greater than or equal to %d", 1);
		return NULL;
	}
	if (start <= 0) {
		PROM_WARN("start must be greater than %d", 0);
		return NULL;
	}
	if (factor <= 1) {
		PROM_WARN("factor must be greater than %d", 1);
		return NULL;
	}

	phb_t *self = (phb_t *) prom_malloc(sizeof(phb_t));
	if (self == NULL)
		return NULL;
	self->upper_bound = NULL;
	self->key = NULL;
	double *upper_bounds = (double *) prom_malloc(sizeof(double) * count);
	const char **keys = (const char **) prom_malloc(sizeof(char *) * count);
	if (upper_bounds == NULL || keys == NULL) {
		phb_destroy(self);
		return NULL;
	}
	upper_bounds[0] = start;
	keys[0] = double_to_str(start);
	for (size_t i = 1; i < count; i++) {
		upper_bounds[i] = upper_bounds[i - 1] * factor;
		keys[i] = double_to_str(upper_bounds[i]);
	}
	self->upper_bound = upper_bounds;
	self->key = keys;
	self->count = count;
	return self;
}

int
phb_destroy(phb_t *self) {
	if (self == NULL)
		return 0;
	for (int i=0; i < self->count; i++)
		free((char *) self->key[i]);
	prom_free((double *) self->upper_bound);
	prom_free((char **) self->key);
	prom_free(self);
	return 0;
}

size_t
phb_count(phb_t *self) {
	PROM_ASSERT(self != NULL);
	return self->count;
}
