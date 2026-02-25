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
#include <stdatomic.h>

void
test_metric_with_no_labels(void) {
	prom_metric_t *metric = prom_metric_new(PROM_GAUGE, "test_counter",
		"counter under test", 0, NULL);
	pms_t *sample = pms_from_labels(metric, NULL);
	pms_set(sample, 1.0);
	sample = pms_from_labels(metric, NULL);
	TEST_ASSERT_EQUAL_DOUBLE(1.0, atomic_load(&sample->r_value));
	prom_metric_destroy(metric);
	metric = NULL;
}

void
test_metric_sample_from_labels(void) {
	prom_metric_t *metric = prom_metric_new(PROM_GAUGE, "test_metric",
		"test counter", 2, (const char *[]) {"foo", "bar"});
	const char *values[] = {"bing", "bang"};
	pms_t *sample = pms_from_labels(metric, values);
	pms_set(sample, 1.0);
	sample = pms_from_labels(metric, values);
	TEST_ASSERT_EQUAL_DOUBLE(1.0, atomic_load(&sample->r_value));
	prom_metric_destroy(metric);
	metric = NULL;
}

int
main(int argc, const char **argv) {
	UNITY_BEGIN();
	RUN_TEST(test_metric_with_no_labels);
	RUN_TEST(test_metric_sample_from_labels);
	return UNITY_END();
}
