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

#include "prom_test_helpers.h"

void
test_pll_append(void) {
	pll_t *l = pll_new();
	TEST_ASSERT(l);
	pll_set_free_fn(l, pll_no_op_free);

	char *one = "one";
	char *two = "two";
	char *three = "three";

	pll_append(l, one);
	pll_append(l, two);
	pll_append(l, three);

	pll_node_t *current_node = l->head;
	char *current_str = (char *) current_node->item;
	TEST_ASSERT_EQUAL_STRING("one", current_str);
	current_node = current_node->next;
	current_str = current_node->item;
	TEST_ASSERT_EQUAL_STRING("two", current_str);
	current_node = current_node->next;
	current_str = current_node->item;
	TEST_ASSERT_EQUAL_STRING("three", current_str);

	pll_destroy(l);
}

void
test_pll_push(void) {
	pll_t *l = pll_new();
	TEST_ASSERT(l);
	pll_set_free_fn(l, pll_no_op_free);

	char *one = "one";
	char *two = "two";
	char *three = "three";

	pll_push(l, one);
	pll_push(l, two);
	pll_push(l, three);

	pll_node_t *current_node = l->head;
	char *current_str = (char *) current_node->item;
	TEST_ASSERT_EQUAL_STRING("three", current_str);
	current_node = current_node->next;
	current_str = current_node->item;
	TEST_ASSERT_EQUAL_STRING("two", current_str);
	current_node = current_node->next;
	current_str = current_node->item;
	TEST_ASSERT_EQUAL_STRING("one", current_str);

	pll_destroy(l);
}

static
pll_compare_t compare_fn(void *item_a, void *item_b) {
	const char *str_a = (const char *) item_a;
	const char *str_b = (const char *) item_b;
	return strcmp(str_a, str_b);
}

void
test_pll_remove(void) {

	pll_t *list = pll_new();
	pll_set_free_fn(list, pll_no_op_free);
	pll_set_compare_fn(list, compare_fn);

	pll_append(list, "node_a");
	pll_append(list, "node_b");
	pll_append(list, "node_c");

	pll_remove(list, "node_b");
	const char *result_a = list->head->item;
	TEST_ASSERT_EQUAL_STRING("node_a", result_a);
	const char *result_c = list->head->next->item;
	TEST_ASSERT_EQUAL_STRING("node_c", result_c);

	pll_destroy(list);
}

int
main(int argc, const char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_pll_append);
	RUN_TEST(test_pll_push);
	RUN_TEST(test_pll_remove);
	return UNITY_END();
}
