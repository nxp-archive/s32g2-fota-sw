/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#include <signal.h>
#include <sys/time.h>
#include <stdio.h>
#include <pthread.h>

#include "pl_dlist.h"
#include "pl_timer.h"
#include "pl_errno.h"

#define STATE_ADD    0x11
#define STATE_START  0x22
#define STATE_READY  0x44
#define STATE_STOP   0x55
#define STATE_DEL    0x66

struct linux_timer {
	struct dlist list;
	uint32_t delta;
	uint32_t count;
	void (*callback)(void *arg);
	void *arg;
	uint32_t state;
};

enum module_state {
	S_NOT_INIT,
	S_STOP,
	S_WORKING
};
struct linux_timer_mgr {
	enum module_state state;
	struct dlist waiting_list;
	struct dlist ready_list;
	pthread_mutex_t mutex;
	atomic_t disable_cnt;
};

struct linux_timer_mgr g_linux_timer;

void show_linux_timer(void)
{
	struct linux_timer *p_ltm;
	struct itimerval curr_val;

	(void)getitimer(ITIMER_REAL, &curr_val);
	printf("sec, usec = %ld, %ld >> \n", curr_val.it_value.tv_sec, curr_val.it_value.tv_usec);
	dlist_for_each_entry(p_ltm,&g_linux_timer.waiting_list,list) {
		printf("delta = %u, count = %u\n", p_ltm->delta, p_ltm->count);
	}
}

static void start_sys_timer(uint32_t delay)
{
	struct linux_timer *p_ltm;
	uint32_t step;
	
	if (dlist_empty(&g_linux_timer.waiting_list))
		return;

	p_ltm = container_of(g_linux_timer.waiting_list.next,struct linux_timer,list);
	step = p_ltm->delta + delay;
	if (!step)
		step++;
	
	p_ltm->delta= 0;
	
	{
		struct itimerval new_value= {
			.it_value = {
				.tv_sec = step / 1000,
				.tv_usec = step % 1000 * 1000,
				},
		};

		g_linux_timer.state = S_WORKING;
		if (setitimer(ITIMER_REAL, &new_value, 0) == -1) {
			perror("setitimer start");
			g_linux_timer.state = S_STOP;
		}
	}
}

static void insert_timer(struct linux_timer *p_new_ltm)
{
	struct linux_timer *p_ltm;

	p_new_ltm->delta = p_new_ltm->count;
	dlist_for_each_entry(p_ltm,&g_linux_timer.waiting_list,list) {
		if (p_new_ltm->delta < p_ltm->delta) {
			dlist_add(&p_new_ltm->list, p_ltm->list.prev, &p_ltm->list);
			p_ltm->delta -= p_new_ltm->delta;
			p_new_ltm->state = STATE_START;
			break;
		}
		p_new_ltm->delta -= p_ltm->delta;
	}

	if (p_new_ltm->state != STATE_START) {
		dlist_add_tail(&p_new_ltm->list, &g_linux_timer.waiting_list);
		p_new_ltm->state = STATE_START;
	}
}

static void linux_alarm_handler(int num)
{
	struct linux_timer *p_linuxtimer, *p_linuxtimer_next;
	
	if (g_linux_timer.state == S_NOT_INIT)
		return;
		
	if (dlist_empty(&g_linux_timer.waiting_list))
		return;

	dlist_for_each_entry_safe(p_linuxtimer, p_linuxtimer_next, &g_linux_timer.waiting_list, list) {
		if (p_linuxtimer->delta)
			break;
		
		dlist_del(&p_linuxtimer->list);
		p_linuxtimer->state = STATE_READY;
		dlist_add_tail(&p_linuxtimer->list, &g_linux_timer.ready_list);
	}
	
	dlist_for_each_entry_safe(p_linuxtimer, p_linuxtimer_next, &g_linux_timer.ready_list, list) {
		dlist_del(&p_linuxtimer->list);
		p_linuxtimer->state = STATE_STOP;
		p_linuxtimer->callback(p_linuxtimer->arg);
	}
	//show_linux_timer();
	start_sys_timer(0);
}

static void  __attribute__ ((constructor)) linux_timer_init(void)
{
	if (g_linux_timer.state != S_NOT_INIT)
		return;
	
	init_dlist_head(&g_linux_timer.waiting_list);
	init_dlist_head(&g_linux_timer.ready_list);
	pthread_mutex_init(&g_linux_timer.mutex, NULL);
	if (signal(SIGALRM, linux_alarm_handler) == SIG_ERR)
		perror("signal:");

	g_linux_timer.disable_cnt = 0;
	g_linux_timer.state = S_STOP;
}

static void pl_timer_callback(void *arg)
{
	struct pl_timer *p_timer = arg;
	
	if (!p_timer->func)
		return;
	p_timer->func(p_timer->arg);
	if (p_timer->repeat)
		insert_timer((struct linux_timer *)p_timer->sys_space);
}

ret_t pl_add_timer(struct pl_timer *p_timer)
{
	struct linux_timer *p_new_ltm;
	
	if (!p_timer)
		return -EINVAL;

	if (!p_timer->func || !p_timer->period)
		return -EINVAL;

	p_new_ltm = (struct linux_timer *)p_timer->sys_space;
	p_new_ltm->state = STATE_ADD;
	return RET_OK;
}

static uint32_t stop_sys_timer(void)
{
	//struct itimerval new_value;
	struct itimerval old_val;
	
	//new_value.it_value.tv_sec = 0;
	//new_value.it_value.tv_usec = 0;
	if (setitimer(ITIMER_REAL, NULL, &old_val)) {
		perror("setitimer stop");
		return 0;
	}
	
	return old_val.it_value.tv_sec * 1000 + old_val.it_value.tv_usec / 1000;
}

static void enable_timer(void)
{
	if (0 == atomic_dec_return(&g_linux_timer.disable_cnt))
		start_sys_timer(0);
}

static void disable_timer(void)
{
	uint32_t remain;
	
	if (1 == atomic_inc_return(&g_linux_timer.disable_cnt)) {
		remain = stop_sys_timer();
		if (g_linux_timer.waiting_list.next != &g_linux_timer.waiting_list) {
			struct linux_timer *p_ltm = container_of(g_linux_timer.waiting_list.next, struct linux_timer, list);

			p_ltm->delta += remain;
		}
	}
}

ret_t pl_start_timer(struct pl_timer *p_timer, uint32_t new_period)
{
	struct linux_timer *p_new_ltm;
	
	if (!p_timer)
		return -EINVAL;

	if (!p_timer->func || !p_timer->period)
		return -EINVAL;

	if (new_period)
		p_timer->period = new_period;

	p_new_ltm = (struct linux_timer *)p_timer->sys_space;

	pthread_mutex_lock(&g_linux_timer.mutex);
	disable_timer();
	
	/* step 1. remove timer if it has started */
	if (p_new_ltm->state == STATE_START)
		dlist_del(&p_new_ltm->list);
	
	/* step 2. init timer */
	p_new_ltm = (struct linux_timer *)p_timer->sys_space;
	p_new_ltm->count = p_timer->period;
	p_new_ltm->callback = pl_timer_callback;
	p_new_ltm->arg = p_timer;

	/* step 3. compare and insert timer */
	insert_timer(p_new_ltm);
	
	/* step 4. restart sys timer */
	enable_timer();
	pthread_mutex_unlock(&g_linux_timer.mutex);
	return RET_OK;
}


void pl_del_timer(struct pl_timer *p_timer)
{
	struct linux_timer *p_ltm;
	struct linux_timer *p_ltm_next;
	
	if (!p_timer)
		return;

	p_ltm = (struct linux_timer *)p_timer->sys_space;
		
	pthread_mutex_lock(&g_linux_timer.mutex);
	disable_timer();
	
	if (p_ltm->state == STATE_START) {
		if (p_ltm->list.next != &g_linux_timer.waiting_list) {
			p_ltm_next = container_of(p_ltm->list.next, struct linux_timer,list);
			p_ltm_next += p_ltm->delta;
		}
		dlist_del(&p_ltm->list);
	}
	
	enable_timer();
	pthread_mutex_unlock(&g_linux_timer.mutex);
	
	p_ltm->state = STATE_DEL;
}

