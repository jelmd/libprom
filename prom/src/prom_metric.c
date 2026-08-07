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

#include <pthread.h>

// Public
#include "prom_alloc.h"
#include "prom_histogram_buckets.h"

// Private
#include "prom_assert.h"
#include "prom_errors.h"
#include "prom_log.h"
#include "prom_map_i.h"
#include "prom_metric_formatter_i.h"
#include "prom_metric_i.h"
#include "prom_metric_sample_histogram_i.h"
#include "prom_metric_sample_i.h"

const char *prom_metric_type_map[5] =
	{ "counter", "gauge", "histogram", "summary", "untyped" };

prom_metric_t *
prom_metric_new(prom_metric_type_t metric_type, const char *name,
	const char *help, size_t label_key_count, const char **label_keys)
{
	prom_metric_t *self = (prom_metric_t *) prom_malloc(sizeof(prom_metric_t));
	if (self == NULL)
		return NULL;
	self->type = metric_type;
	self->name = name;
	self->help = help;
	self->buckets = NULL;
	self->formatter = NULL;

	const char **k = (const char **)
		prom_malloc(sizeof(const char *) * label_key_count);

	for (int i = 0; i < label_key_count; i++) {
		if (strcmp(label_keys[i], "le") == 0) {
			PROM_WARN(PROM_METRIC_INVALID_LABEL_NAME "(%s)", "le");
			goto fail;
		}
		if (strcmp(label_keys[i], "quantile") == 0) {
			PROM_WARN(PROM_METRIC_INVALID_LABEL_NAME "(%s)", "quantile");
			goto fail;
		}
		k[i] = prom_strdup(label_keys[i]);
	}
	self->label_keys = k;
	self->label_key_count = label_key_count;
	self->samples = prom_map_new();

	if (metric_type == PROM_HISTOGRAM) {
		if (prom_map_set_free_value_fn(self->samples,
			&pms_histogram_free_generic))
		{
			goto fail;
		}
	} else if (prom_map_set_free_value_fn(self->samples, &pms_free_generic)) {
		goto fail;
	}

	if ((self->formatter = pmf_new()) == NULL)
		goto fail;

	self->rwlock = (pthread_rwlock_t *) prom_malloc(sizeof(pthread_rwlock_t));
	if (self->rwlock == NULL || pthread_rwlock_init(self->rwlock, NULL) != 0) {
		PROM_WARN(PROM_PTHREAD_RWLOCK_INIT_ERROR, NULL);
		return NULL;
	}
	return self;

fail:
	prom_metric_destroy(self);
	return NULL;
}

int
prom_metric_destroy(prom_metric_t *self) {
	if (self == NULL)
		return 0;

	phb_destroy(self->buckets);
	self->buckets = NULL;

	prom_map_destroy(self->samples);
	self->samples = NULL;

	pmf_destroy(self->formatter);
	self->formatter = NULL;

	if (pthread_rwlock_destroy(self->rwlock))
		PROM_WARN(PROM_PTHREAD_RWLOCK_DESTROY_ERROR, NULL);

	prom_free(self->rwlock);
	self->rwlock = NULL;

	for (int i = 0; i < self->label_key_count; i++) {
		prom_free((void *) self->label_keys[i]);
		self->label_keys[i] = NULL;
	}
	prom_free(self->label_keys);
	self->label_keys = NULL;

	prom_free(self);
	return 0;
}

int
prom_metric_destroy_generic(void *item) {
	return prom_metric_destroy((prom_metric_t *) item);
}

void
prom_metric_free_generic(void *item) {
	prom_metric_destroy((prom_metric_t *) item);
}

pms_t *
pms_from_labels(prom_metric_t *self, const char **label_values) {
	PROM_ASSERT(self != NULL);
	if (pthread_rwlock_wrlock(self->rwlock)) {
		PROM_WARN(PROM_PTHREAD_RWLOCK_LOCK_ERROR, NULL);
		return NULL;
	}

	// Get l_value
	if (pmf_load_l_value(self->formatter, self->name, NULL,
		self->label_key_count, self->label_keys, label_values))
	{
		goto fail;
	}

	// This must be freed before returning
	const char *l_value = pmf_dump(self->formatter);
	if (l_value == NULL)
		goto fail;

	// Get sample
	pms_t *sample = (pms_t *) prom_map_get(self->samples, l_value);
	if (sample == NULL) {
		sample = pms_new(self->type, l_value, 0.0);
		if (prom_map_set(self->samples, l_value, sample))
			goto fail;
	}
	pthread_rwlock_unlock(self->rwlock);
	prom_free((void *) l_value);
	return sample;

fail:
    if (l_value != NULL)
        prom_free((void *) l_value);
	if (pthread_rwlock_unlock(self->rwlock))
		PROM_WARN(PROM_PTHREAD_RWLOCK_UNLOCK_ERROR, NULL);
	return NULL;
}

pms_histogram_t *
pms_histogram_from_labels(prom_metric_t *self, const char **label_values) {
	PROM_ASSERT(self != NULL);
	if (pthread_rwlock_wrlock(self->rwlock)) {
		PROM_WARN(PROM_PTHREAD_RWLOCK_LOCK_ERROR, NULL);
		return NULL;
	}

	// Load the l_value
	if (pmf_load_l_value(self->formatter, self->name, NULL,
		self->label_key_count, self->label_keys, label_values))
	{
		goto fail;
	}

	// This must be freed before returning
	const char *l_value = pmf_dump(self->formatter);
	if (l_value == NULL)
		goto fail;

	// Get sample
	pms_histogram_t *sample = (pms_histogram_t *)
		prom_map_get(self->samples, l_value);
	if (sample == NULL) {
		sample = pms_histogram_new(self->name, self->buckets,
			self->label_key_count, self->label_keys, label_values);
		if (sample == NULL || prom_map_set(self->samples, l_value, sample)) {
			prom_free((void *) l_value);
			goto fail;
		}
	}
	pthread_rwlock_unlock(self->rwlock);
	prom_free((void *) l_value);
	return sample;

fail:
	if (pthread_rwlock_unlock(self->rwlock))
		PROM_WARN(PROM_PTHREAD_RWLOCK_UNLOCK_ERROR, NULL);
	return NULL;
}
