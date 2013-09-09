#pragma once

/*
 * Author: Fuang.Cao
 * Email: cavan.cfa@gmail.com
 * Date: Mon Dec 17 15:10:18 CST 2012
 */

#include <cavan.h>
#include <cavan/network.h>
#include <cavan/service.h>

#define WEB_PROXY_TEMP_FILE		CAVAN_TEMP_PATH "/cavan-XXXXXX.html"

typedef enum
{
	HTTP_REQ_CONNECT,
	HTTP_REQ_DELETE,
	HTTP_REQ_GET,
	HTTP_REQ_HEAD,
	HTTP_REQ_OPTIONS,
	HTTP_REQ_PUT,
	HTTP_REQ_POST,
	HTTP_REQ_TRACE,
} http_request_type_t;

struct tcp_proxy_service
{
	struct cavan_service_description service;
	int sockfd;
	u16 port;
	u16 proxy_port;
	char proxy_ip[32];

	int (*open_connect)(const char *ip, u16 port);
	void (*close_connect)(int sockfd);
};

int tcp_proxy_service_run(struct tcp_proxy_service *proxy_service);
int web_proxy_get_request_type(const char *req);
int web_proxy_service_run(struct cavan_service_description *service, u16 port);
