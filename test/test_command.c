/*
 * File:		testCommand.c
 * Author:		Fuang.Cao <cavan.cfa@gmail.com>
 * Created:		2015-09-23 16:55:59
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
#include <cavan/command.h>

__maybe_unused static void test_cmdline_parse(char *cmdline)
{
	int i;
	int argc;
	char *argv[10];

	argc = cavan_cmdline_parse(cmdline, argv, NELEM(argv));

	for (i = 0; i < argc; i++) {
		println("argv[%d] = %s", i, argv[i]);
	}
}

__maybe_unused static void test_async_command_handler1(void *data)
{
	pr_func_info("data = %s", (char *) data);
}

__maybe_unused static void test_async_command_handler2(void *data)
{
	pr_func_info("data = %s", (char *) data);
}

int main(int argc, char *argv[])
{
#if 0
	int ret;
	pid_t pid;
	int size[2];
	int ttyfds[3];
	int flags = CAVAN_EXECF_STDIN | CAVAN_EXECF_STDOUT | CAVAN_EXECF_STDERR;

	assert(argc > 1);

	tty_get_win_size(0, size);

	while (1) {
		ret = cavan_exec_redirect_stdio_popen2(argv[1], size[0], size[1], &pid, flags);
		if (ret < 0) {
			pr_red_info("cavan_exec_redirect_stdio_popen2");
			return ret;
		}

		ret = cavan_exec_open_temp_pipe_slave(ttyfds, pid, flags);
		if (ret < 0) {
			pr_red_info("cavan_exec_open_temp_pipe_client: %d", ret);
			return ret;
		}

		cavan_tty_redirect(ttyfds[0], ttyfds[1], ttyfds[2]);

		cavan_exec_close_temp_pipe(ttyfds, -1);
	}
#else
	assert(argc > 1);

#if 0
	test_cmdline_parse(argv[1]);
#elif 1
	for (optind = 1; optind < argc; optind++) {
		if (optind & 1) {
			cavan_async_command_execute(NULL, test_async_command_handler1, argv[optind], optind * 1000);
		} else {
			cavan_async_command_execute(NULL, test_async_command_handler2, argv[optind], optind * 1000);
		}
	}

	println("cancel = %d", cavan_async_command_cancel(NULL, test_async_command_handler2, 0));

	msleep(argc * 1000 + 2000);

	for (optind = 1; optind < argc; optind++) {
		cavan_async_command_execute(NULL, test_async_command_handler2, argv[optind], 1000);
		msleep(2000);
	}
#else
	cavan_pipe_cmdline_loop3(argv[1], NULL, 0, NULL);
#endif

#endif

	return 0;
}
