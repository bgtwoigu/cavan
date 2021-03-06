/*
 * File:		touchscreen.c
 * Based on:
 * Author:		Fuang Cao <cavan.cfa@gmail.com>
 *
 * Created:		2012-11-17
 * Description:	HUAMOBILE LIBRARY
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

#include <huamobile/touchscreen.h>
#include <huamobile/input.h>

bool huamobile_multi_touch_device_match(uint8_t *abs_bitmask)
{
	if (test_bit(ABS_MT_POSITION_X, abs_bitmask) == 0 || test_bit(ABS_MT_POSITION_Y, abs_bitmask) == 0) {
		return false;
	}

	clean_bit(ABS_MT_POSITION_X, abs_bitmask);
	clean_bit(ABS_MT_POSITION_Y, abs_bitmask);

	return true;
}

bool huamobile_multi_touch_device_matcher(int fd, const char *name, void *data)
{
	int ret;
	uint8_t abs_bitmask[ABS_BITMASK_SIZE];

	pr_pos_info();

	ret = huamobile_event_get_abs_bitmask(fd, abs_bitmask);
	if (ret < 0) {
		pr_error_info("huamobile_event_get_abs_bitmask");
		return false;
	}

	return huamobile_multi_touch_device_match(abs_bitmask);
}

bool huamobile_single_touch_device_match(uint8_t *abs_bitmask, uint8_t *key_bitmask)
{
	if (test_bit(BTN_TOUCH, key_bitmask) == 0 || test_bit(ABS_X, abs_bitmask) == 0 || test_bit(ABS_Y, abs_bitmask) == 0) {
		return false;
	}

	clean_bit(BTN_TOUCH, key_bitmask);
	clean_bit(ABS_X, abs_bitmask);
	clean_bit(ABS_Y, abs_bitmask);

	return true;
}

bool huamobile_single_touch_device_matcher(int fd, const char *name, void *data)
{
	int ret;
	uint8_t abs_bitmask[ABS_BITMASK_SIZE];
	uint8_t key_bitmask[KEY_BITMASK_SIZE];

	pr_pos_info();

	ret = huamobile_event_get_abs_bitmask(fd, abs_bitmask);
	if (ret < 0) {
		pr_error_info("huamobile_event_get_abs_bitmask");
		return false;
	}

	ret = huamobile_event_get_key_bitmask(fd, key_bitmask);
	if (ret < 0) {
		pr_error_info("huamobile_event_get_key_bitmask");
		return false;
	}

	return huamobile_single_touch_device_match(abs_bitmask, key_bitmask);
}

bool huamobile_touch_device_matcher(int fd, const char *name, void *data)
{
	int ret;
	uint8_t abs_bitmask[ABS_BITMASK_SIZE];
	uint8_t key_bitmask[KEY_BITMASK_SIZE];

	pr_pos_info();

	ret = huamobile_event_get_abs_bitmask(fd, abs_bitmask);
	if (ret < 0) {
		pr_error_info("huamobile_event_get_abs_bitmask");
		return false;
	}

	if (huamobile_multi_touch_device_match(abs_bitmask)) {
		pr_green_info("Deivce %s mutil touch screen", name);
		return true;
	}

	ret = huamobile_event_get_key_bitmask(fd, key_bitmask);
	if (ret < 0) {
		pr_error_info("huamobile_event_get_key_bitmask");
		return false;
	}

	return huamobile_single_touch_device_match(abs_bitmask, key_bitmask);
}

static inline void huamobile_touch_point_mapping(struct huamobile_touch_device *dev, struct huamobile_touch_point *point)
{
	point->x = point->x * dev->xscale - dev->xoffset;
	point->y = point->y * dev->yscale - dev->yoffset;
}

static int huamobile_touch_device_probe(struct huamobile_touch_device *dev, void *data)
{
	int ret;
	int min, max, diff;
	int fd = dev->input_dev.event_dev->fd;
	struct huamobile_input_service *service = data;

	pr_pos_info();
	pr_bold_info("lcd_width = %d, lcd_height = %d", service->lcd_width, service->lcd_height);

	if (service->lcd_width > 0) {
		ret = huamobile_event_get_absinfo(fd, dev->xaxis, &min, &max);
		if (ret < 0) {
			pr_red_info("huamobile_event_get_absinfo");
			return ret;
		}

		pr_bold_info("x-min = %d, x-max = %d", min, max);
		diff = max - min;
		dev->xscale = ((double) service->lcd_width) / diff;
		dev->xoffset = ((double) service->lcd_width) * min / diff;
	} else {
		dev->xscale = 1;
		dev->xoffset = 0;
	}

	if (service->lcd_height > 0) {
		ret = huamobile_event_get_absinfo(fd, dev->yaxis, &min, &max);
		if (ret < 0) {
			pr_red_info("huamobile_event_get_absinfo");
			return ret;
		}

		pr_bold_info("y-min = %d, y-max = %d", min, max);
		diff = max - min;
		dev->yscale = ((double) service->lcd_height) / diff;
		dev->yoffset = ((double) service->lcd_height) * min / diff;
	} else {
		dev->yscale = 1;
		dev->yoffset = 0;
	}

	pr_bold_info("xscale = %lf, xoffset = %lf", dev->xscale, dev->xoffset);
	pr_bold_info("yscale = %lf, yoffset = %lf", dev->yscale, dev->yoffset);

	return 0;
}

static bool huamobile_multi_touch_event_handler(struct huamobile_input_device *dev, struct input_event *event, void *data)
{
	struct huamobile_multi_touch_device *ts = (struct huamobile_multi_touch_device *) dev;
	struct huamobile_input_service *service = data;
	struct huamobile_touch_point *p, *p_end;

	switch (event->type) {
	case EV_ABS:
		switch (event->code) {
		case ABS_MT_WIDTH_MAJOR:
			break;

		case ABS_MT_POSITION_X:
			ts->points[ts->point_count].x = event->value;
			break;

		case ABS_MT_POSITION_Y:
			ts->points[ts->point_count].y = event->value;
			break;

		case ABS_MT_TOUCH_MAJOR:
			ts->points[ts->point_count].pressure = event->value;
			break;

		case ABS_MT_TRACKING_ID:
			ts->points[ts->point_count].id = event->value;
			break;

		default:
			return false;
		}
		break;

	case EV_SYN:
		switch (event->code) {
		case SYN_MT_REPORT:
			p = ts->points + ts->point_count;
			if (p->pressure >= 0) {
				ts->point_count++;
			}
			break;

		case SYN_REPORT:
			if (ts->point_count) {
				struct huamobile_virtual_key *key;

				for (p = ts->points, p_end = p + ts->point_count; p < p_end; p++) {
					key = huamobile_event_find_virtual_key(dev->event_dev, p->x, p->y);
					if (key) {
						int value = p->pressure > 0;

						if (key->value != value) {
							service->key_handler(dev, key->name, key->code, value, service->private_data);
							key->value = value;
						}

						ts->point_count--;
					} else if (p->pressure > 0) {
						huamobile_touch_point_mapping(&ts->touch_dev, p);

						if (p->released) {
							service->touch_handler(dev, p, service->private_data);
							p->released = 0;
						}

						service->move_handler(dev, p, service->private_data);
					} else {
						ts->point_count--;
					}

					p->pressure = -1;
				}
			} else {
				struct huamobile_virtual_key *key;

				for (key = dev->event_dev->vk_head; key; key = key->next) {
					if (key->value) {
						service->key_handler(dev, key->name, key->code, 0, service->private_data);
						key->value = 0;
					}
				}
			}

			if (ts->point_count < ts->point_count_old) {
				for (p = ts->points + ts->point_count, p_end = p + ts->point_count_old; p < p_end; p++) {
					if (p->released == 0) {
						service->release_handler(dev, p, service->private_data);
						p->released = 1;
					}
				}
			}

			ts->point_count_old = ts->point_count;
			ts->point_count = 0;
			break;

		default:
			return false;
		}
		break;

	default:
		return false;
	}

	return true;
}

static int huamobile_multi_touch_device_probe(struct huamobile_input_device *dev, void *data)
{
	struct huamobile_multi_touch_device *ts = (struct huamobile_multi_touch_device *) dev;

	return huamobile_touch_device_probe(&ts->touch_dev, data);
}

struct huamobile_input_device *huamobile_multi_touch_device_create(void)
{
	struct huamobile_multi_touch_device *ts;
	struct huamobile_touch_device *touch_dev;
	struct huamobile_input_device *dev;
	struct huamobile_touch_point *p, *p_end;

	pr_pos_info();

	ts = malloc(sizeof(*ts));
	if (ts == NULL) {
		pr_error_info("malloc");
		return NULL;
	}

	ts->point_count = 0;
	ts->point_count_old = 0;

	for (p = ts->points, p_end = p + NELEM(ts->points); p < p_end; p++) {
		p->id = p - ts->points;
		p->pressure = -1;
		p->released = 1;
	}

	touch_dev = &ts->touch_dev;
	touch_dev->xaxis = ABS_MT_POSITION_X;
	touch_dev->yaxis = ABS_MT_POSITION_Y;

	dev = &touch_dev->input_dev;
	dev->probe = huamobile_multi_touch_device_probe;
	dev->remove = NULL;
	dev->event_handler = huamobile_multi_touch_event_handler;

	return dev;
}

// ================================================================================

static bool huamobile_single_touch_event_handler(struct huamobile_input_device *dev, struct input_event *event, void *data)
{
	struct huamobile_single_touch_device *ts = (struct huamobile_single_touch_device *) dev;
	struct huamobile_input_service *service = data;
	struct huamobile_touch_point *p = &ts->point;

	switch (event->type) {
	case EV_ABS:
		switch (event->code) {
		case ABS_X:
			p->x = event->value;
			break;

		case ABS_Y:
			p->y = event->value;
			break;

		case ABS_PRESSURE:
			p->pressure = event->value;
			break;

		default:
			return false;
		}
		break;

	case EV_SYN:
		if (ts->pressed) {
			struct huamobile_virtual_key *key;
			key = huamobile_event_find_virtual_key(dev->event_dev, p->x, p->y);
			if (key) {
				int value = p->pressure > 0;

				if (key->value != value) {
					service->key_handler(dev, key->name, key->code, value, service->private_data);
				}

				key->value = value;
			} else {
				huamobile_touch_point_mapping(&ts->touch_dev, p);

				if (p->released) {
					service->touch_handler(dev, p, service->private_data);
					p->released = 0;
				}

				service->move_handler(dev, p, service->private_data);
				p->released = 0;
			}
		} else {
			struct huamobile_virtual_key *key;

			for (key = dev->event_dev->vk_head; key; key = key->next) {
				if (key->value != 0) {
					service->key_handler(dev, key->name, key->code, 0, service->private_data);
				}

				key->value = 0;
			}

			if (p->released == 0) {
				service->release_handler(dev, p, service->private_data);
				p->released = 1;
			}
		}
		break;

	default:
		return false;
	}

	return true;
}

static int huamobile_single_touch_device_probe(struct huamobile_input_device *dev, void *data)
{
	struct huamobile_single_touch_device *ts = (struct huamobile_single_touch_device *) dev;

	return huamobile_touch_device_probe(&ts->touch_dev, data);
}

struct huamobile_input_device *huamobile_single_touch_device_create(void)
{
	struct huamobile_single_touch_device *ts;
	struct huamobile_touch_device *touch_dev;
	struct huamobile_input_device *dev;

	pr_pos_info();

	ts = malloc(sizeof(*ts));
	if (ts == NULL) {
		pr_error_info("malloc");
		return NULL;
	}

	ts->pressed = 0;
	ts->point.released = 1;

	touch_dev = &ts->touch_dev;
	touch_dev->xaxis = ABS_X;
	touch_dev->yaxis = ABS_Y;

	dev = &touch_dev->input_dev;
	dev->probe = huamobile_single_touch_device_probe;
	dev->remove = NULL;
	dev->event_handler = huamobile_single_touch_event_handler;

	return dev;
}
