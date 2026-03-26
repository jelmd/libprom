/*
Copyright 2019 DigitalOcean Inc.
Copyright 2021 Jens Elkner <jel+libprom@cs.uni-magdeburg.de>

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/**
 * @file prom.h
 * @brief Include prom.h to include the entire public API
 * @mainpage libprom documentation
 * @tableofcontents
 * @section Introduction
 *
 * libprom is a small suite of C libraries which can be used to maintain
 * metrics on-the-fly and to expose them automatically in the Prometheus
 * exposition format when needed.
 *
 * In this brief tutorial you will learn how to create and register metrics,
 * update metric samples, and expose metrics over HTTP.
 *
 * In the old original version awefully long function and type names were used.
 * They got renamed for better readability and more compact code. Now the
 * following prefix abbreviation are used instead of:
 *
 *	* pcr .. prom_collector_registry
 *	* phb .. prom_histogram_buckets
 *	* pll .. prom_linked_list
 *	* pmf .. prom_metric_formatter
 *	* pms .. prom_metric_sample
 *	* ppl .. prom_process_limits
 *	* pps .. prom_process_stat[s]
 *	* psb .. prom_string_builder
 *	* ppc .. prom_process_collector (formerly prom_collector_process)
 *
 * @section Creating-and-Registering-Metrics Creating and Registering Metrics
 *
 * First thing to do is to create the prom collector registry.
 * A prom collector registry is basically an associative array of prom
 * collectors keyed by their names. It is recommended to use the default
 * prom collector registry (it has the name "default"). Use
 * \c pcr_init() to set it up. Under the hood it also
 * initializes the \c default prom collector, where all your prom metrics alias
 * counters, gauges, histograms will be added by default. If \c PROM_PROCESS
 * gets
 * passed as an argument, a prom collector named \c process gets initialized
 * as well, which contains and collects several metrics about the running
 * process/thread. A prom collector is basically an associative array of prom
 * metrics keyed by their names.
 *
 * After that, write a metric initialization function, which creates the
 * required metrics and registers them with the prom collector of choice.
 * Usually one would use the \c pcr_register_metric() function: It will add
 * the metrics to the \c default prom collector which is registered with the
 * \c default prom collector registry.
 *
 * If one has created his own, non-default prom collector registry using
 * \c pcr_registry_new(), one may use
 * \c pcr_registry_get() with the name parameter set to \c default
 * to get a reference to its default collector instance, or create a new
 * collector using \c pcr_new() and register it with the registry
 * via \c pcr_register_collector(). To add metrics to the
 * related container, use \c prom_collector_add_metric().
 *
 * However, here an easy example, which uses the default prom collector registry
 * including the \c "process" collector:
 *
 * @code{.c}
 *
 * #incldue "prom.h"
 *
 * prom_counter_t *my_counter;
 *
 * void foo_metric_init(void) {
 *      my_counter = prom_counter_new("my_counter", "counts things", 0, NULL);
 *      if (my_counter != NULL && pcr_register_metric(my_counter))
 *         PROM_INFO("metric '%s' registered.", my_counter->name);
 * }
 *
 * // prefix all metric names with "myapp_" and report the overall scrape time
 * if (pcr_init(PROM_PROCESS|PROM_SCRAPETIME, "myapp_")) {
 *     foo_metric_init();
 * }
 * @endcode
 *
 * If you need more metrics, add them as needed. libprom supports metrics with
 * the following metric types:
 *
 * * [Counter](https://prometheus.io/docs/concepts/metric_types/#counter)
 * * [Gauge](https://prometheus.io/docs/concepts/metric_types/#gauge)
 * * [Histogram](https://prometheus.io/docs/concepts/metric_types/#histogram)
 *
 *
 * @section Updating-Metric-Sample-Values Updating Metric Sample Values
 *
 * Now that we have a metric created and registered our metric(s), we can
 * update its value as needed. For example:
 *
 * @code{.c}
 *
 * void my_lib_do_something(void) {
 *   printf("I did a really important thing!\n");
 *   prom_counter_inc(my_counter, NULL);
 * }
 * @endcode
 *
 * This function will increment the default metric sample for my_counter. Since
 * we are not using metric labels, we pass \c NULL as the second argument.
 *
 *
 * @section Metric-Exposition-Over-HTTP Metric Exposition Over HTTP
 *
 * To expose all the metrics of all enabled collectors in a given prom
 * collector registry via HTTP, one needs to activate the registry using
 * promhttp_set_active_collector_registry() - if the passed registry is \c NULL,
 * the default prom collector registry will be used instead. Note: Only one
 * registry at a time is active.
 *
 * After firing up the HTTP handler via \c promhttp_start_daemon(), the
 * HTTP handler calls the \c pcr_bridge() on \c /metrics
 * request. This in turn instructs the registry's prom metric formatter (pmf)
 * to reset its internal stringbuilder, i.e. clear its buffer via
 * \c pmf_clear() and populate the response by calling
 * \c pmf_load_metrics(). This function calls the
 * \c collect_fn() function of each registered collector to get the list of
 * metrics to include in the HTTP response. Finally the formatter iterates
 * through the returned list of metrics, and appends them in Prometheus
 * exposition format to its internal stringbuilder. When all prom collectors of
 * the registry have been processed, the contents of the formmatter's
 * stringbuilder gets dumped as a single string via
 * \c pmf_dump() and append to the HTTP response of the
 * request - the http handler takes care of the rest.
 *
 * So basically do something like this:
 *
 * @code{.c}
 *
 *  promhttp_set_active_collector_registry(NULL);
 *
 *  struct MHD_Daemon *daemon = promhttp_start_daemon(MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL);
 *  if (daemon == NULL)
 *    return 1;
 * @endcode
 *
 * @section collect_fn collect_fn()
 * As explained above, to dump registered metrics in Prometheus exposition
 * format, the formatter calls the \c collect_fn() function of all registered
 * prom collectors and is per default the integral part when generating the
 * response for a \c /metrics request. So what does it, is expected to do?
 *
 * For any \c default prom collector \c collect_fn() does nothing but just
 * returns the list of registered metrics unless one had set another function
 * to call using \c prom_collector_set_collect_fn(). However, the \c process
 * prom collector automatically created and registered via
 * \c pcr_init(PROM_PROCESS, ...) reads in all process
 * related
 * data now, updates the values of the relevant metrics and after that it
 * returns the list of metrics to include in the response as well.
 *
 * So the take-away here is, that collect_fn() may trigger other things like
 * metric updates, too. But since it is per default part of the HTTP response
 * generation, it should be very fast to prevent any timeouts. Depending on
 * the [flags](https://www.gnu.org/software/libmicrohttpd/manual/html_node/microhttpd_002dconst.html#microhttpd_002dconst#index-MHD_005fFLAG)
 * passed to the \c promhttp_start_daemon() call the http daemon may answers
 * any request one after another.
 *
 * @section faq FAQ
 * I do not want to maintain any metric on-the-fly?
 *
 * So you only want to convert some collected data to Prometheus exposition
 * format and export it via HTTP? In this case you do not need libprom but
 * just need to have a look at [promhttp.c](https://github.com/jelmd/libprom/blob/main/promhttp/src/promhttp.c).
 * Copy-and-paste and change the line, where
 * \c pcr_bridge(PROM_ACTIVE_REGISTRY) gets called. Replace
 * it with a call to your own function which should return the desired metrics
 * as a string. That's it.
 *
 * But I want to re-use the metrics maintained by the "process" collector.
 *
 * In this case you can do the same as above and use
 * \c pcr_init(PROM_PROCESS, ...) to initialize the default
 * prom registry
 * \c PROM_COLLECTOR_REGISTRY. When it is time to expose the metrics just
 * call \c pcr_bridge(PROM_COLLECTOR_REGISTRY) and append
 * the returned string to the output of your own export.
 *
 *
 * @section Where-To-Go-From-Here Where to Go From Here?
 *
 * Take a look at the [Files](./files.html)
 * tab in this documentation site for more information about the public API available to you. Also, you can take a look
 * at the examples directory at the
 * [Github repository](https://github.com/jelmd/libprom/tree/main/example) for inspiration.
 */

#ifndef PROM_INCLUDED
#define PROM_INCLUDED

#include "prom_alloc.h"
#include "prom_collector.h"
#include "prom_collector_registry.h"
#include "prom_counter.h"
#include "prom_gauge.h"
#include "prom_histogram.h"
#include "prom_histogram_buckets.h"
#include "prom_linked_list.h"
#include "prom_map.h"
#include "prom_metric.h"
#include "prom_metric_sample.h"
#include "prom_metric_sample_histogram.h"

#endif //  PROM_INCLUDED
