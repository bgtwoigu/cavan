/*
 * File:		mux.c
 * Author:		Fuang.Cao <cavan.cfa@gmail.com>
 * Created:		2015-07-28 11:43:37
 *
 * Copyright (c) 2015 Fuang.Cao <cavan.cfa@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <cavan.h>
#include <cavan/mux.h>

static int cavan_mux_recv_thread_handler(struct cavan_thread *thread, void *data)
{
	char *p;
	ssize_t rdlen, wrlen;
	struct cavan_mux *mux = data;
	char buff[CAVAN_MUX_MTU + sizeof(struct cavan_mux_package)];

	rdlen = mux->recv(mux, buff, sizeof(buff));
	if (rdlen < 0) {
		pr_red_info("mux->recv");
		return rdlen;
	}

	p = buff;

	while (rdlen > 0) {
		wrlen = cavan_mux_append_receive_data(mux, p, rdlen);
		if (wrlen < 0) {
			pr_red_info("cavan_mux_append_receive_data");
			return wrlen;
		}

		p += wrlen;
		rdlen -= wrlen;
	}

	return 0;
}

static int cavan_mux_send_thread_handler(struct cavan_thread *thread, void *data)
{
	struct cavan_mux *mux = data;
	struct cavan_mux_package_raw *package_raw;

	package_raw = mux->package_head;
	if (package_raw == NULL) {
		cavan_thread_suspend(thread);
	} else {
		struct cavan_mux_package *package = &package_raw->package;
		char *data = (char *) package;
		size_t length = cavan_mux_package_get_whole_length(package);

		while (length) {
			ssize_t wrlen;

			wrlen = mux->send(mux, data, length);
			if (wrlen < 0) {
				pr_red_info("mux->send");
				return wrlen;
			}

			data += wrlen;
			length -= wrlen;
		}

		mux->package_head = package_raw->next;
		if (mux->package_head == NULL) {
			mux->package_tail = &mux->package_head;
		}

		cavan_mux_package_free(mux, package);
	}

	return 0;
}

int cavan_mux_init(struct cavan_mux *mux, void *data)
{
	int i;
	int ret;
	struct cavan_thread *thread;

	if (mux->send == NULL || mux->recv == NULL) {
		pr_red_info("mux->send = %p, mux->recv = %p", mux->send, mux->recv);
		return -EINVAL;
	}

	cavan_lock_init(&mux->lock, false);

	ret = cavan_mem_queue_init(&mux->recv_queue, CAVAN_MUX_MTU);
	if (ret < 0) {
		pr_red_info("cavan_mem_queue_init: %d", ret);
		goto out_cavan_lock_deinit;
	}

	mux->private_data = data;
	mux->packages = NULL;

	mux->package_head = NULL;
	mux->package_tail = &mux->package_head;

	for (i = 0; i < NELEM(mux->links); i++) {
		mux->links[i] = NULL;
	}

	thread = &mux->recv_thread;
	thread->name = "MUX RECEIVE";
	thread->handler = cavan_mux_recv_thread_handler;
	thread->wake_handker = NULL;
	ret = cavan_thread_run(thread, mux, 0);
	if (ret < 0) {
		pr_red_info("cavan_thread_run");
		goto out_cavan_mem_queue_deinit;
	}

	thread = &mux->send_thread;
	thread->name = "MUX SEND";
	thread->handler = cavan_mux_send_thread_handler;
	thread->wake_handker = NULL;
	ret = cavan_thread_run(thread, mux, 0);
	if (ret < 0) {
		pr_red_info("cavan_thread_run");
		goto out_cavan_thread_stop_recv;
	}

	return 0;

out_cavan_thread_stop_recv:
	cavan_thread_stop(&mux->recv_thread);
out_cavan_mem_queue_deinit:
	cavan_mem_queue_deinit(&mux->recv_queue);
out_cavan_lock_deinit:
	cavan_lock_deinit(&mux->lock);
	return ret;
}

void cavan_mux_deinit(struct cavan_mux *mux)
{
	struct cavan_mux_package_raw *head;

	cavan_thread_stop(&mux->recv_thread);

	cavan_lock_acquire(&mux->lock);

	head = mux->packages;
	while (head) {
		struct cavan_mux_package_raw *next = head->next;

		free(head);
		head = next;
	}

	cavan_lock_release(&mux->lock);

	cavan_lock_deinit(&mux->lock);
}

void cavan_mux_append_package(struct cavan_mux *mux, struct cavan_mux_package_raw *package)
{
	struct cavan_mux_package_raw **head;

	cavan_lock_acquire(&mux->lock);

	for (head = &mux->packages; *head && (*head)->length < package->length; head = &(*head)->next);

	package->next = *head;
	*head = package;

	cavan_lock_release(&mux->lock);
}

struct cavan_mux_package_raw *cavan_mux_dequeue_package(struct cavan_mux *mux, size_t length)
{
	struct cavan_mux_package_raw **head;
	struct cavan_mux_package_raw *package;

	cavan_lock_acquire(&mux->lock);

	head = &mux->packages;

	while (*head && (*head)->length < length) {
		head = &(*head)->next;
	}

	package = *head;
	if (package) {
		*head = package->next;
	}

	cavan_lock_release(&mux->lock);

	return package;
}

void cavan_mux_show_packages(struct cavan_mux *mux)
{
	struct cavan_mux_package_raw *package;

	cavan_lock_acquire(&mux->lock);

	for (package = mux->packages; package; package = package->next) {
		println("length = %d", package->length);
	}

	cavan_lock_release(&mux->lock);
}

struct cavan_mux_package *cavan_mux_package_alloc(struct cavan_mux *mux, size_t length)
{
	struct cavan_mux_package *package;
	struct cavan_mux_package_raw *package_raw;

	package_raw = cavan_mux_dequeue_package(mux, length);
	if (package_raw == NULL) {
		package_raw = malloc(sizeof(struct cavan_mux_package_raw) + length);
		if (package_raw == NULL) {
			pr_error_info("malloc");
			return NULL;
		}

		package_raw->length = length;
	}

	package = &package_raw->package;
	package->length = length;

	return package;
}

void cavan_mux_package_free(struct cavan_mux *mux, struct cavan_mux_package *package)
{
	cavan_mux_append_package(mux, CAVAN_MUX_PACKAGE_GET_RAW(package));
}

int cavan_mux_add_link(struct cavan_mux *mux, struct cavan_mux_link *link, u16 port)
{
	struct cavan_mux_link **head;

	cavan_lock_acquire(&mux->lock);

	head = mux->links + (cavan_mux_link_head_index(port));
	while ((*head) && (*head)->local_port < port) {
		head = &(*head)->next;
	}

	if ((*head) && (*head)->local_port == port) {
		pr_red_info("port %d already exists!", port);
		cavan_lock_release(&mux->lock);
		return -EINVAL;
	}

	link->local_port = port;

	link->next = *head;
	*head = link;

	cavan_lock_release(&mux->lock);

	return 0;
}

struct cavan_mux_link *cavan_mux_find_link(struct cavan_mux *mux, u16 port)
{
	struct cavan_mux_link *link;

	cavan_lock_acquire(&mux->lock);

	for (link = mux->links[cavan_mux_link_head_index(port)]; link && link->local_port != port; link = link->next);

	cavan_lock_release(&mux->lock);

	return link;
}

void cavan_mux_unbind(struct cavan_mux *mux, struct cavan_mux_link *link)
{
	struct cavan_mux_link **head;

	cavan_lock_acquire(&mux->lock);

	head = mux->links + (cavan_mux_link_head_index(link->local_port));
	if (link == *head) {
		*head = link->next;
	} else {
		struct cavan_mux_link *prev;

		for (prev = *head; prev && prev->next != link; prev = prev->next);

		if (prev) {
			prev->next = link->next;
		}
	}

	cavan_lock_release(&mux->lock);
}

int cavan_mux_find_free_port(struct cavan_mux *mux, u16 *pport)
{
	int i, j;
	int ret = 0;

	cavan_lock_acquire(&mux->lock);

	for (i = 0; i < NELEM(mux->links); i++) {
		struct cavan_mux_link *head = mux->links[i];

		for (j = 0; j < (1 << (sizeof(u16) << 3)) / NELEM(mux->links); j++) {
			u16 port = j * NELEM(mux->links) + i;

			if (head && head->local_port == port) {
				head = head->next;
			} else if (port != 0) {
				*pport = port;
				goto label_found;
			}
		}
	}

	ret = -EBUSY;

label_found:
	cavan_lock_release(&mux->lock);
	return ret;
}

int cavan_mux_bind(struct cavan_mux *mux, struct cavan_mux_link *link, u16 port)
{
	int ret;

	cavan_lock_acquire(&mux->lock);

	if (port == 0) {
		ret = cavan_mux_find_free_port(mux, &port);
		if (ret < 0) {
			cavan_lock_release(&mux->lock);
			return ret;
		}
	}

	ret = cavan_mux_add_link(mux, link, port);

	cavan_lock_release(&mux->lock);

	return ret;
}

int cavan_mux_append_receive_package(struct cavan_mux *mux, struct cavan_mux_package *package)
{
	int ret;
	struct cavan_mux_link *link;

	cavan_lock_acquire(&mux->lock);

	link = cavan_mux_find_link(mux, package->dest_port);
	if (link == NULL) {
		pr_red_info("invalid port %d", package->dest_port);
		ret = -EINVAL;
	} else {
		ret = cavan_mux_link_append_receive_package(link, package);;
	}

	cavan_lock_release(&mux->lock);

	return ret;
}

ssize_t cavan_mux_append_receive_data(struct cavan_mux *mux, const void *buff, size_t size)
{
	int ret;
	size_t wrlen, rdlen;
	struct cavan_mux_package package;
	struct cavan_mux_package *ppackage;

	cavan_lock_acquire(&mux->lock);

	wrlen = cavan_mem_queue_inqueue(&mux->recv_queue, buff, size);

	while (1) {
		rdlen = cavan_mem_queue_dequeue_peek(&mux->recv_queue, &package, sizeof(package));
		if (rdlen < sizeof(package)) {
			goto out_success;
		}

		if (package.magic == CAVAN_MUX_MAGIC) {
			break;
		}

		cavan_mem_queue_dequeue(&mux->recv_queue, NULL, 1);
	}

	size = cavan_mux_package_get_whole_length(&package);
	if (cavan_mem_queue_get_used_size(&mux->recv_queue) < size) {
		goto out_success;
	}

	ppackage = cavan_mux_package_alloc(mux, package.length);
	if (ppackage == NULL) {
		pr_red_info("cavan_mux_package_alloc");
		ret = -ENOMEM;
		goto out_cavan_lock_release;
	}

	if (cavan_mem_queue_dequeue(&mux->recv_queue, ppackage, size) != size) {
		pr_red_info("cavan_mem_queue_dequeue");
		ret = -EFAULT;
		goto out_cavan_lock_release;
	}

	ret = cavan_mux_append_receive_package(mux, ppackage);
	if (ret < 0) {
		pr_red_info("cavan_mux_write_recv_package: %d", ret);
		cavan_mux_package_free(mux, ppackage);
	}

out_success:
	ret = wrlen;
out_cavan_lock_release:
	cavan_lock_release(&mux->lock);
	return ret;
}

void cavan_mux_append_send_package(struct cavan_mux *mux, struct cavan_mux_package *package)
{
	struct cavan_mux_package_raw *package_raw = CAVAN_MUX_PACKAGE_GET_RAW(package);

	package->magic = CAVAN_MUX_MAGIC;

	cavan_lock_acquire(&mux->lock);

	*mux->package_tail = package_raw;
	package_raw->next = NULL;
	mux->package_tail = &package_raw->next;
	cavan_thread_resume(&mux->send_thread);

	cavan_lock_release(&mux->lock);
}

// ================================================================================

void cavan_mux_link_init(struct cavan_mux_link *link, struct cavan_mux *mux)
{
	cavan_lock_init(&link->lock, false);

	link->mux = mux;
	link->hole_size = 0;
	link->package_head = NULL;
	link->package_tail = &link->package_head;
}

void cavan_mux_link_deinit(struct cavan_mux_link *link)
{
	cavan_lock_deinit(&link->lock);
}

int cavan_mux_link_append_receive_package(struct cavan_mux_link *link, struct cavan_mux_package *package)
{
	struct cavan_mux_package_raw *package_raw = CAVAN_MUX_PACKAGE_GET_RAW(package);

	cavan_lock_acquire(&link->lock);

	*link->package_tail = package_raw;
	package_raw->next = NULL;
	link->package_tail = &package_raw->next;

	link->remote_port = package->src_port;

	if (link->on_received) {
		link->on_received(link);
	}

	cavan_lock_release(&link->lock);

	return 0;
}

ssize_t cavan_mux_link_recv(struct cavan_mux_link *link, void *buff, size_t size)
{
	size_t length;
	const char *data;
	struct cavan_mux_package *package;
	struct cavan_mux_package_raw *package_raw;

	cavan_lock_acquire(&link->lock);

	package_raw = link->package_head;
	if (package_raw == NULL) {
		cavan_lock_release(&link->lock);
		return 0;
	}

	package = &package_raw->package;
	data = package->data + link->hole_size;
	length = package->length - link->hole_size;
	if (size < length) {
		memcpy(buff, data, size);

		link->hole_size += size;
	} else {
		size = length;
		memcpy(buff, data, size);

		cavan_mux_package_free(link->mux, package);
		link->hole_size = 0;
	}

	cavan_lock_release(&link->lock);

	return size;
}

ssize_t cavan_mux_link_send(struct cavan_mux_link *link, const void *buff, size_t size)
{
	struct cavan_mux_package *package;
	struct cavan_mux *mux = link->mux;

	package = cavan_mux_package_alloc(mux, size);
	if (package == NULL) {
		pr_red_info("cavan_mux_package_alloc");
		return -ENOMEM;
	}

	cavan_lock_acquire(&link->lock);

	package->src_port = link->local_port;
	package->dest_port = link->remote_port;

	cavan_lock_release(&link->lock);

	memcpy(package->data, buff, size);

	cavan_mux_append_send_package(mux, package);

	return size;
}
