/*
 * Author: Fuang.Cao
 * Email: cavan.cfa@gmail.com
 * Date: Mon Apr 16 10:03:27 CST 2012
 */

#include <cavan.h>
#include <cavan/timer.h>
#include <cavan/service.h>
#include <cavan/process.h>
#include <cavan/command.h>
#include <cavan/android.h>
#include <cavan/permission.h>

#define CAVAN_SERVICE_DEBUG		0

int cavan_dynamic_service_count;
struct cavan_dynamic_service *cavan_dynamic_services[10];

static const char *cavan_locale_envs[] = {
	"LC_PAPER",
	"LC_ADDRESS",
	"LC_MONETARY",
	"LC_NUMERIC",
	"LC_TELEPHONE",
	"LC_IDENTIFICATION",
	"LC_MEASUREMENT",
	"LC_TIME",
	"LC_NAME",
	"LC_ALL",
	"LANG",
	"LANGUAGE",
};

static void *cavan_service_handler(void *data)
{
	struct cavan_service_description *desc = data;
	static int count = 0;
	int index;

	pthread_mutex_lock(&desc->mutex_lock);
	index = ++count;
	pthread_mutex_unlock(&desc->mutex_lock);

	while (1) {
		int ret;

		pd_bold_info("%s daemon %d ready", desc->name, index);

		ret = desc->handler(desc, index, desc->data);
		if (ret < 0) {
			pd_red_info("%s daemon %d fault", desc->name, index);
		} else {
			pd_green_info("%s daemon %d complete", desc->name, index);
		}
	}

	return NULL;
}

void cavan_service_set_busy(struct cavan_service_description *desc, int index, bool busy)
{
	pthread_mutex_lock(&desc->mutex_lock);

	if (busy) {
		desc->used_count++;
	} else {
		desc->used_count--;
	}

	pd_green_info("%s daemon %d %s [%d/%d]", desc->name, index, busy ? "busy" : "idle", desc->used_count, desc->daemon_count);

	pthread_mutex_unlock(&desc->mutex_lock);
}

int cavan_service_start(struct cavan_service_description *desc)
{
	int ret;
	int i;
	int count;
	pthread_t *threads;
	const char *homepath;

	if (desc == NULL) {
		pr_red_info("desc == NULL");
		ERROR_RETURN(EINVAL);
	}

	if (desc->handler == NULL || desc->name == NULL || desc->daemon_count < 1) {
		pr_red_info("desc->handler == NULL || desc->name == NULL || desc->daemon_count < 1");
		ERROR_RETURN(EINVAL);
	}

	if (desc->super_permission && (ret = check_super_permission(true, 5000)) < 0) {
		return ret;
	}

	if (desc->as_daemon) {
		pd_blue_info("Run %s as daemon", desc->name);
		ret = daemon(0, desc->show_verbose);
		if (ret < 0) {
			pr_err_info("daemon");
			return ret;
		}
	}

	count = desc->daemon_count;
	threads = (pthread_t *) malloc(sizeof(pthread_t) * count);
	if (threads == NULL) {
		pr_red_info("malloc");
		return -ENOMEM;
	}

	ret = pthread_mutex_init(&desc->mutex_lock, NULL);
	if (ret < 0) {
		pr_red_info("pthread_mutex_init");
		goto out_free_threads;
	}

	homepath = getenv("HOME");
	if (homepath) {
		ret = chdir(homepath);
	}

	umask(0);
	desc->used_count = 0;

	for (i = 0; i < count; i++) {
		ret = cavan_pthread_create(threads + i, cavan_service_handler, desc, true);
		if (ret < 0) {
			pr_red_info("cavan_pthread_create");

			while (i-- > 0) {
#ifndef CONFIG_ANDROID
				pthread_cancel(threads[i]);
#endif
				free(threads);
				return ret;
			}
		}
	}

	desc->threads = threads;

	return 0;

out_free_threads:
	free(threads);
	return ret;
}

int cavan_service_main_loop(struct cavan_service_description *desc)
{
	int i;
	pthread_t *threads;

	for (i = desc->daemon_count - 1, threads = desc->threads; i >= 0; i--) {
		pthread_join(threads[i], NULL);
	}

	return 0;
}

int cavan_service_run(struct cavan_service_description *desc)
{
	int ret;

	ret = cavan_service_start(desc);
	if (ret < 0) {
		pr_red_info("cavan_service_start");
		return ret;
	}

	return cavan_service_main_loop(desc);
}

int cavan_service_stop(struct cavan_service_description *desc)
{
	int i;
	pthread_t *threads;

	if (desc == NULL) {
		pr_red_info("desc == NULL");
		return -EINVAL;
	}

	threads = desc->threads;
	if (threads == NULL) {
		return 0;
	}

	for (i = desc->daemon_count - 1; i >= 0; i--) {
#ifndef CONFIG_ANDROID
		pthread_cancel(threads[i]);
#endif
		pthread_kill(threads[i], SIGUSR1);
	}

	free(threads);
	desc->threads = NULL;

	return 0;
}

int cavan_daemon_run(struct cavan_daemon_description *desc)
{
	int ret;

	if (desc == NULL || desc->command == NULL) {
		pr_red_info("desc == NULL || desc->command == NULL");
		ERROR_RETURN(EINVAL);
	}

	pd_bold_info("command = %s", desc->command);

	if (desc->super_permission) {
		ret = check_super_permission(true, 5000);
		if (ret < 0) {
			return ret;
		}
	} else {
		ret = setuid(getuid());
		ret = setgid(getgid());
	}

	if (desc->logfile) {
		ret = cavan_stdio_redirect3(desc->logfile, 0x06);
		if (ret < 0) {
			pr_red_info("cavan_redirect_stdio");
			return ret;
		}

		desc->verbose = 1;
	}

	if (desc->as_daemon) {
		ret = daemon(1, desc->verbose);
		if (ret < 0) {
			pr_error_info("daemon");
			return ret;
		}

		if (desc->pidfile) {
			file_printf(desc->pidfile, "%d", getpid());
		}
	}

	return cavan_exec_command(desc->command, 0, NULL);
}

int cavan_daemon_stop(struct cavan_daemon_description *desc)
{
	pid_t pid;

	if (desc == NULL) {
		pr_red_info("desc == NULL");
		return -EINVAL;
	}

	if (desc->pidfile) {
		char buff[1024];
		ssize_t readlen;

		readlen = file_read(desc->pidfile, buff, sizeof(buff));
		if (readlen < 0) {
			pr_red_info("Read file %s failed", desc->pidfile);
			return readlen;
		}

		remove(desc->pidfile);

		pid = text2value_unsigned(buff, NULL, 10);
	} else if (desc->command) {
		pid = process_find_by_cmdline(NULL, desc->command);
		if (pid < 0) {
			pr_red_info("process_find_by_cmdline failed");
			return pid;
		}
	} else {
		pr_red_info("Please give command pathname or pid filename");
		return -EINVAL;
	}

	return kill(pid, SIGTERM);
}

// ================================================================================

int cavan_dynamic_service_init(struct cavan_dynamic_service *service)
{
	int ret;

	ret = pthread_mutex_init(&service->lock, NULL);
	if (ret < 0) {
		pr_error_info("pthread_mutex_init");
		return ret;
	}

	ret = pthread_cond_init(&service->cond, NULL);
	if (ret < 0) {
		pr_error_info("pthread_cond_init");
		goto out_pthread_mutex_destroy;
	}

	service->min = 10;
	service->max = 1000;

	return 0;

out_pthread_mutex_destroy:
	pthread_mutex_destroy(&service->lock);
	return ret;
}

void cavan_dynamic_service_deinit(struct cavan_dynamic_service *service)
{
	pthread_cond_destroy(&service->cond);
	pthread_mutex_destroy(&service->lock);
}

struct cavan_dynamic_service *cavan_dynamic_service_create(size_t size)
{
	struct cavan_dynamic_service *service;

	service = malloc(sizeof(*service) + size);
	if (service == NULL) {
		pr_error_info("malloc");
		return NULL;
	}

	memset(service, 0, sizeof(*service));

	if (cavan_dynamic_service_init(service) < 0) {
		pr_red_info("cavan_dynamic_service_init");
		goto out_free_service;
	}

	service->private_data = service + 1;

	return service;

out_free_service:
	free(service);
	return NULL;
}

void cavan_dynamic_service_destroy(struct cavan_dynamic_service *service)
{
	cavan_dynamic_service_deinit(service);
	free(service);
}

static void *cavan_dynamic_service_handler(void *data)
{
	int ret;
	u32 index;
	void *conn;
	struct cavan_dynamic_service *service = data;

	while (1) {
		conn = malloc(service->conn_size);
		if (conn) {
			break;
		}

		pr_error_info("malloc");
		msleep(100);
	}

	pthread_mutex_lock(&service->lock);

	service->count++;
	index = ++service->index;

	while (service->state == CAVAN_SERVICE_STATE_RUNNING) {
		pd_bold_info("service %s daemon %d ready (%d/%d)", service->name, index, service->used, service->count);

		pthread_mutex_unlock(&service->lock);
		ret = service->open_connect(service, conn);
		pthread_mutex_lock(&service->lock);
		if (ret < 0) {
			pr_red_info("open_connect");
			continue;
		}

		service->used++;

		if (service->used < service->count) {
			pd_green_info("don't need create daemon");
		} else {
			if (service->count < service->max) {
				int ret;

				ret = cavan_pthread_run(cavan_dynamic_service_handler, service);
				if (ret < 0) {
					pd_red_info("create daemon faild");
				} else {
					pd_green_info("create daemon successfully");
				}
			} else {
				pd_red_info("too match deamon count = %d", service->count);
			}
		}

		pd_bold_info("service %s daemon %d running (%d/%d)", service->name, index, service->used, service->count);

		pthread_mutex_unlock(&service->lock);
		ret = service->run(service, conn);
		service->close_connect(service, conn);
		if (ret < 0) {
			pd_red_info("service %s daemon %d fault", service->name, index);
		} else {
			pd_green_info("service %s daemon %d complete", service->name, index);
		}

		pthread_mutex_lock(&service->lock);
		service->used--;

		if (service->count - service->used > service->min) {
			break;
		}
	}

	service->count--;
	pd_green_info("service %s daemon %d exit (%d/%d)", service->name, index, service->used, service->count);

	if (service->count == 0) {
		pd_red_info("service %s stopped", service->name);

		service->state = CAVAN_SERVICE_STATE_STOPPED;

		pthread_cond_signal(&service->cond);
	}

	pthread_mutex_unlock(&service->lock);

	free(conn);

	return NULL;
}

static void cavan_dynamic_service_wait_stop_complete(struct cavan_dynamic_service *service)
{
	int i;

	pthread_mutex_lock(&service->lock);

	for (i = 0; i < 20 && service->stopping; i++) {
		service->state = CAVAN_SERVICE_STATE_STOPPED;
		pthread_mutex_unlock(&service->lock);
		msleep(100);
		pthread_mutex_lock(&service->lock);
	}

	pthread_mutex_unlock(&service->lock);
}

int cavan_dynamic_service_start(struct cavan_dynamic_service *service, bool sync)
{
	int i;
	int ret;
	struct passwd *pw;
	const char *homepath;

	if (service == NULL) {
		pr_red_info("service == NULL");
		return -EINVAL;
	}

	if (service->name == NULL) {
		pr_red_info("service->name == NULL");
		return -EINVAL;
	}

	if (service->open_connect == NULL) {
		pr_red_info("service->open_connect == NULL");
		return -EINVAL;
	}

	if (service->close_connect == NULL) {
		pr_red_info("service->close_connect == NULL");
		return -EINVAL;
	}

	if (service->start == NULL) {
		pr_red_info("service->start == NULL");
		return -EINVAL;
	}

	if (service->stop == NULL) {
		pr_red_info("service->stop == NULL");
		return -EINVAL;
	}

	if (service->run == NULL) {
		pr_red_info("service->run == NULL");
		return -EINVAL;
	}

	pd_bold_info("service %s daemon (%d/%d)", service->name, service->min, service->max);

	if (service->min <= 0 || service->max < service->min) {
		pr_red_info("invalid min or max");
		return -EINVAL;
	}

	if (service->user) {
		pw = cavan_user_get_passwd(service->user);
		if (pw == NULL) {
			pr_red_info("invalid user: %s", service->user);
			return -EINVAL;
		}

		ret = setuid(pw->pw_uid);
		if (ret < 0) {
			pr_err_info("setuid: %d", ret);
			return ret;
		}

		service->super_permission = 0;
	} else {
		pw = cavan_user_get_passwd(NULL);
	}

	if (service->group) {
		struct group *gr = cavan_group_get_group(service->group);
		if (gr == NULL) {
			pr_red_info("invalid group: %s", service->group);
			return -EINVAL;
		}

		ret = setgid(gr->gr_gid);
		if (ret < 0) {
			pr_err_info("setgid: %d", ret);
			return ret;
		}

		service->super_permission = 0;
	}

	if (service->super_permission && (ret = check_super_permission(true, 5000)) < 0) {
		return ret;
	}

	umask(0);

	cavan_stdio_redirect3("/dev/null", service->verbose ? 0x01 : 0x07);

	if (service->logfile) {
		ret = cavan_stdio_redirect3(service->logfile, 0x06);
		if (ret < 0) {
			pr_red_info("cavan_redirect_stdio");
			return ret;
		}

		service->verbose = true;

		println("PATH = %s", getenv("PATH"));
	}

	if (pw != NULL) {
		setenv("USER", pw->pw_name, 1);
		setenv("LOGNAME", pw->pw_name, 1);

		if (pw->pw_dir) {
			setenv("HOME", pw->pw_dir, file_access_w(pw->pw_dir));
		}

		if (pw->pw_shell) {
			setenv("SHELL", pw->pw_shell, 1);
		}
	}

#ifdef CONFIG_ANDROID
	{
		char buff[64];

		if (android_get_hostname(buff, sizeof(buff)) > 0) {
			setenv("HOSTNAME", buff, 0);
		}
	}
#endif

	if (service->as_daemon) {
		pd_blue_info("Run %s as daemon", service->name);

		ret = daemon(1, service->verbose);
		if (ret < 0) {
			pr_red_info("daemon");
			return ret;
		}

		sync = true;
	} /* else {
		setsid();
	} */

	cavan_exec_set_oom_adj2(0, 0);

	homepath = getenv("HOME");
	if (homepath) {
		pd_bold_info("change current work directory to %s", homepath);
		ret = chdir(homepath);
	}

	setenv("PS1", "\\[\\e]0;${debian_chroot:+($debian_chroot)}\\u@\\h: \\w\\a\\]$ ", 1);
	setenv("PROMPT_COMMAND", "echo -ne \"\\033]0;${USER}@${HOSTNAME}: ${PWD/$HOME/~}\\007\"", 1);
	setenv("TERM", "xterm-color", 1);

	for (i = 0; i < NELEM(cavan_locale_envs); i++) {
		setenv(cavan_locale_envs[i], "zh_CN.UTF-8", 1);
	}

	service->count = 0;
	service->used = 0;
	service->index = 0;
	service->stopping = false;
	service->state = CAVAN_SERVICE_STATE_RUNNING;

	ret = service->start(service);
	if (ret < 0) {
		pr_red_info("service->start");
		return ret;
	}

	if (service->conn_size <= 0) {
		ret = -EINVAL;
		pr_red_info("invalid conn_size = %" PRINT_FORMAT_SIZE, service->conn_size);
		goto out_service_stop;
	}

	pd_bold_info("conn_size = %" PRINT_FORMAT_SIZE, service->conn_size);

	if (sync) {
		cavan_dynamic_service_register(service);
		cavan_dynamic_service_handler(service);
		cavan_dynamic_service_wait_stop_complete(service);
		cavan_dynamic_service_unregister(service);
	} else {
		int i;

		ret = cavan_pthread_run(cavan_dynamic_service_handler, service);
		if (ret < 0) {
			pr_error_info("cavan_pthread_create");
			goto out_service_stop;
		}

		pthread_mutex_lock(&service->lock);

		for (i = 0; i < 200; i++) {
			pthread_mutex_unlock(&service->lock);
			msleep(10);
			pthread_mutex_lock(&service->lock);

			if (service->count) {
				break;
			}

			pd_bold_info("service %s not ready", service->name);
		}

		pthread_mutex_unlock(&service->lock);

		cavan_dynamic_service_register(service);
	}

	return 0;

out_service_stop:
	service->stop(service);
	return ret;
}

void cavan_dynamic_service_join(struct cavan_dynamic_service *service)
{
	pthread_mutex_lock(&service->lock);

	while (service->count) {
		pd_bold_info("service %s daemon count %d", service->name, service->count);
		pthread_cond_wait(&service->cond, &service->lock);
	}

	if (service->state == CAVAN_SERVICE_STATE_RUNNING) {
		service->stop(service);
	}

	service->state = CAVAN_SERVICE_STATE_STOPPED;

	pthread_mutex_unlock(&service->lock);

	cavan_dynamic_service_wait_stop_complete(service);
	cavan_dynamic_service_unregister(service);

	pd_bold_info("service %s stopped", service->name);
}

int cavan_dynamic_service_run(struct cavan_dynamic_service *service)
{
	int ret = cavan_dynamic_service_start(service, true);
	if (ret < 0) {
		pr_red_info("cavan_dynamic_service_start");
		return ret;
	}

	cavan_dynamic_service_join(service);

	return 0;
}

int cavan_dynamic_service_stop(struct cavan_dynamic_service *service)
{
	pthread_mutex_lock(&service->lock);

	service->stopping = true;

	if (service->state == CAVAN_SERVICE_STATE_RUNNING) {
		int i;

		service->state = CAVAN_SERVICE_STATE_STOPPING;
		service->stop(service);

		for (i = 0; i < 10 && service->count; i++) {
			struct timespec abstime;

			cavan_timer_set_timespec(&abstime, 2000);
			pthread_cond_timedwait(&service->cond, &service->lock, &abstime);

			if (service->state == CAVAN_SERVICE_STATE_STOPPED) {
				break;
			}

			pd_red_info("wait service stop %d", i);
		}
	}

	service->stopping = false;

	pthread_mutex_unlock(&service->lock);

	cavan_dynamic_service_unregister(service);

	return 0;
}

bool cavan_dynamic_service_register(struct cavan_dynamic_service *service)
{
	int i;

	for (i = 0; i < NELEM(cavan_dynamic_services); i++) {
		if (cavan_dynamic_services[i] == NULL) {
			cavan_dynamic_services[i] = service;
			cavan_dynamic_service_count++;
			return true;
		} else if (cavan_dynamic_services[i] == service) {
			return true;
		}
	}

	return false;
}

bool cavan_dynamic_service_unregister(struct cavan_dynamic_service *service)
{
	int i;

	for (i = 0; i < NELEM(cavan_dynamic_services); i++) {
		if (cavan_dynamic_services[i] == service) {
			cavan_dynamic_services[i] = NULL;
			cavan_dynamic_service_count--;
			return true;
		}
	}

	return false;
}

struct cavan_dynamic_service *cavan_dynamic_service_find(const char *name)
{
	int i;

	for (i = 0; i < NELEM(cavan_dynamic_services); i++) {
		struct cavan_dynamic_service *service = cavan_dynamic_services[i];
		if (service && strcmp(service->name, name) == 0) {
			return service;
		}
	}

	return NULL;
}

boolean cavan_dynamic_service_stop_by_name(const char *name)
{
	struct cavan_dynamic_service *service;

	service = cavan_dynamic_service_find(name);
	if (service == NULL) {
		return false;
	}

	cavan_dynamic_service_stop(service);

	return true;
}

void cavan_dynamic_service_scan(void *data, void (*handler)(struct cavan_dynamic_service *service, void *data))
{
	int i;

	for (i = 0; i < NELEM(cavan_dynamic_services); i++) {
		struct cavan_dynamic_service *service = cavan_dynamic_services[i];
		if (service) {
			handler(service, data);
		}
	}
}
