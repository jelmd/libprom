/**
 * Copyright 2019-2020 DigitalOcean Inc.
 * Copyright 2021 Jens Elkner <jel+libprom@cs.uni-magdeburg.de>.
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

#include <stdatomic.h>

// Public
#include "prom_alloc.h"

// Private
#include "prom_assert.h"
#include "prom_errors.h"
#include "prom_log.h"
#include "prom_metric_sample_i.h"
#include "prom_metric_sample_t.h"

pms_t *
pms_new(prom_metric_type_t type, const char *l_val, double r_val) {
	pms_t *self = (pms_t *) prom_malloc(sizeof(pms_t));
	if (self == NULL)
		return NULL;
	self->type = type;
	self->l_value = prom_strdup(l_val);
	self->r_value = ATOMIC_VAR_INIT(r_val);
	return self;
}

int
pms_destroy(pms_t *self) {
	if (self == NULL)
		return 0;
	prom_free((void *) self->l_value);
	self->l_value = NULL;
	prom_free((void *) self);
	return 0;
}

int
pms_destroy_generic(void *gen) {
	return pms_destroy((pms_t *) gen);
}

void
pms_free_generic(void *gen) {
	pms_destroy((pms_t *) gen);
}

int
pms_add(pms_t *self, double r_value) {
	PROM_ASSERT(self != NULL);
	if (r_value < 0)
		return 1;
	double old = atomic_load(&self->r_value);
	for (;;) {
		double desired = ATOMIC_VAR_INIT(old + r_value);
		if (atomic_compare_exchange_weak(&self->r_value, &old, desired))
			return 0;
	}
}

int
pms_sub(pms_t *self, double r_value) {
	PROM_ASSERT(self != NULL);
	if (self->type != PROM_GAUGE) {
		PROM_WARN(PROM_METRIC_INCORRECT_TYPE " (%d) - %s = %s",
			self->type, self->l_value, self->r_value);
		return 1;
	}
	double old = atomic_load(&self->r_value);
	for (;;) {
		double desired = ATOMIC_VAR_INIT(old - r_value);
		if (atomic_compare_exchange_weak(&self->r_value, &old, desired))
			return 0;
	}
}

int
pms_set(pms_t *self, double r_value) {
	if (self->type != PROM_GAUGE && (self->type != PROM_COUNTER || r_value < 0))
	{
		PROM_WARN(PROM_METRIC_INCORRECT_TYPE " (%d) - %s = %s",
			self->type, self->l_value, self->r_value);
		return 1;
	}
	atomic_store(&self->r_value, r_value);
	return 0;
}
