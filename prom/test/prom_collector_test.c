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
#include "prom_process_collector_t.h"

void
test_prom_collector(void) {
	prom_collector_t *c = prom_collector_new("test");
	TEST_ASSERT_NOT_NULL(c);
	prom_counter_t *counter =
		prom_counter_new("test_counter", "counter under test", 0, NULL);
	prom_collector_add_metric(c, counter);
	prom_map_t *m = c->collect_fn(c);
	TEST_ASSERT_EQUAL_INT(1, prom_map_size(m));
	prom_collector_destroy(c);
}

void
test_prom_process_collector(void) {
	prom_collector_t *c =
#ifdef __sun
	ppc_new("../test/fixtures/limits","../test/fixtures/status",0,NULL,NULL);
#else	// assume Linux
	ppc_new("../test/fixtures/limits","../test/fixtures/stat",0,NULL,NULL);
#endif
	TEST_ASSERT_NOT_NULL(c);
	TEST_ASSERT_NOT_NULL(c->data);
	TEST_ASSERT_NOT_NULL(c->free_data_fn);
	prom_map_t *m = c->collect_fn(c);
	TEST_ASSERT_EQUAL_INT_MESSAGE(PM_COUNT, prom_map_size(m),
		"number of metrics");
	prom_collector_destroy(c);
	TEST_ASSERT_NULL(c->free_data_fn);
	TEST_ASSERT_NULL(c->data);
}

int
main(int argc, const char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_prom_collector);
	RUN_TEST(test_prom_process_collector);
	return UNITY_END();
}
