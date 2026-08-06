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

// Public
#include "prom_alloc.h"

// Private
#include "prom_assert.h"
#include "prom_linked_list_i.h"
#include "prom_linked_list_t.h"
#include "prom_log.h"

pll_t *
pll_new(void) {
	pll_t *self = (pll_t *) prom_malloc(sizeof(pll_t));
	if (self == NULL)
		return NULL;
	self->head = NULL;
	self->tail = NULL;
	self->free_fn = NULL;
	self->compare_fn = NULL;
	self->size = 0;
	return self;
}

int
pll_purge(pll_t *self) {
	if (self == NULL)
		return 1;

	pll_node_t *node = self->head;
	while (node != NULL) {
		pll_node_t *next = node->next;
		if (node->item != NULL) {
			if (self->free_fn) {
				(*self->free_fn)(node->item);
			} else {
				prom_free(node->item);
			}
		}
		prom_free(node);
		node = NULL;
		node = next;
	}
	self->head = NULL;
	self->tail = NULL;
	self->size = 0;
	return 0;
}

int
pll_destroy(pll_t *self) {
	PROM_ASSERT(self != NULL);
	pll_purge(self);
	prom_free(self);
	return 0;
}

void *
pll_first(pll_t *self) {
	PROM_ASSERT(self != NULL);
	return (self->head) ? self->head->item : NULL;
}

void *
pll_last(pll_t *self) {
	PROM_ASSERT(self != NULL);
	return (self->tail) ? self->tail->item : NULL;
}

int
pll_append(pll_t *self, void *item) {
	if (self == NULL)
		return 1;
	pll_node_t *node = (pll_node_t *) prom_malloc(sizeof(pll_node_t));
	if (node == NULL)
		return 2;

	node->item = item;
	if (self->tail) {
		self->tail->next = node;
	} else {
		self->head = node;
	}
	self->tail = node;
	node->next = NULL;
	self->size++;
	return 0;
}

int
pll_push(pll_t *self, void *item) {
	if (self == NULL)
		return 1;
	pll_node_t *node = (pll_node_t *) prom_malloc(sizeof(pll_node_t));
	if (node == NULL)
		return 2;

	node->item = item;
	node->next = self->head;
	self->head = node;
	if (self->tail == NULL)
		self->tail = node;
	self->size++;
	return 0;
}

void *
pll_pop(pll_t *self) {
	if (self == NULL)
		return NULL;
	pll_node_t *node = self->head;
	if (node == NULL)
		return NULL;

	void *item = node->item;
	self->head = node->next;
	if (self->tail == node)
		self->tail = NULL;
    prom_free(node);
	self->size--;
	return item;
}

int
pll_remove(pll_t *self, void *item) {
	if (self == NULL)
		return 1;
	pll_node_t *node;
	pll_node_t *prev_node = NULL;

	// Locate the node
	for (node = self->head; node != NULL; node = node->next) {
		if (self->compare_fn) {
			if ((*self->compare_fn)(node->item, item) == PROM_EQUAL)
				break;
		} else if (node->item == item) {
			break;
		}
		prev_node = node;
	}

	if (node == NULL)
		return 0;

	if (prev_node) {
		prev_node->next = node->next;
	} else {
		self->head = node->next;
	}
	if (node->next == NULL)
		self->tail = prev_node;

	if (node->item != NULL) {
		if (self->free_fn) {
			(*self->free_fn)(node->item);
		} else {
			prom_free(node->item);
		}
	}

	node->item = NULL;
	prom_free(node);
	node = NULL;
	self->size--;
	return 0;
}

pll_compare_t
pll_compare(pll_t *self, void *item_a, void *item_b) {
	if (self == NULL)
		return 1;

	return (self->compare_fn == NULL)
		? strcmp(item_a, item_b)
		: (*self->compare_fn)(item_a, item_b);
}

size_t
pll_size(pll_t *self) {
	PROM_ASSERT(self != NULL);
	return self->size;
}

int
pll_set_free_fn(pll_t *self, pll_free_item_fn free_fn) {
	if (self == NULL)
		return 1;
	self->free_fn = free_fn;
	return 0;
}

int
pll_set_compare_fn(pll_t *self, pll_compare_item_fn compare_fn) {
	if (self == NULL)
		return 1;
	self->compare_fn = compare_fn;
	return 0;
}

void
pll_no_op_free(void *item) {
	// no-op
}
