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

#include <string.h>

#include "microhttpd.h"
#include "prom.h"
#include "prom_log.h"

pcr_t *PROM_ACTIVE_REGISTRY;

void
promhttp_set_active_collector_registry(pcr_t *registry) {
	PROM_ACTIVE_REGISTRY = (registry == NULL)
		? PROM_COLLECTOR_REGISTRY
		: registry;
	if (PROM_ACTIVE_REGISTRY == NULL)
		PROM_WARN("No registry set to answer http requests", "");
}

#if MHD_VERSION >= 0x00097500
	enum MHD_Result
#else
	int
#endif
promhttp_handler(void *cls, struct MHD_Connection *connection, const char *url,
	const char *method, const char *version, const char *upload_data,
	size_t *upload_data_size, void **con_cls)
{
	char *body;
	struct MHD_Response *response;
	enum MHD_ResponseMemoryMode mode = MHD_RESPMEM_PERSISTENT;
	unsigned int status = MHD_HTTP_BAD_REQUEST;

#if MHD_VERSION >= 0x00097500
	enum MHD_Result	ret;
#else
	int ret;
#endif

	if (strcmp(method, "GET") != 0) {
		body = "Invalid HTTP Method\n";
	} else if (strcmp(url, "/") == 0) {
		body = "<html><body>See <a href='/metrics'>/metrics</a>.\r\n";
		status = MHD_HTTP_OK;
	} else if (strcmp(url, "/metrics") == 0) {
		body = pcr_bridge(PROM_ACTIVE_REGISTRY);
		mode = MHD_RESPMEM_MUST_FREE;
		status = MHD_HTTP_OK;
	} else {
		body = "Bad Request\n";
	}

	response = MHD_create_response_from_buffer(strlen(body), body, mode);
	if (response == NULL) {
		if (mode == MHD_RESPMEM_MUST_FREE)
			free(body);
		ret = MHD_NO;
	} else {
		ret = MHD_queue_response(connection, status, response);
		MHD_destroy_response(response);
	}
	return ret;
}

struct MHD_Daemon *
promhttp_start_daemon(unsigned int flags, unsigned short port,
	MHD_AcceptPolicyCallback apc, void *apc_cls, ...)
{
	struct MHD_Daemon *daemon;
	va_list ap;

	va_start (ap, apc_cls);
	daemon = MHD_start_daemon_va(flags, port, apc, apc_cls, &promhttp_handler, NULL, ap);
	va_end (ap);
	return daemon;
}

void promhttp_stop_daemon(struct MHD_Daemon *daemon) {
	MHD_stop_daemon(daemon);
}
