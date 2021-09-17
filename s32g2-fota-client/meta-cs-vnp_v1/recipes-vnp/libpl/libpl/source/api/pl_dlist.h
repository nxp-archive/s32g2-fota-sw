/*
 * Copyright 2018-2021 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause

 */
#pragma once
#define FILE__PL_DLIST_H
#include "pl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

struct dlist {
	struct dlist *prev;
	struct dlist *next;
};

#define dlist_head_init(name) { &(name), &(name) }

#define dlist_head(name) \
	struct dlist name = dlist_head_init(name)

static inline void init_dlist_head(struct dlist *head)
{
	head->next = head->prev = head;
}

#define dlist_first_entry(ptr, type, member) container_of((ptr)->next, type, member)

#define dlist_next_entry(pos, member) container_of((pos)->member.next, __typeof__(*(pos)), member)

#define dlist_for_each_entry(pos, head, member)				\
	for (pos = dlist_first_entry(head, __typeof__(*pos), member);	\
	     &pos->member != (head);					\
	     pos = dlist_next_entry(pos, member))


#define dlist_for_each_entry_safe(pos, n, head, member)				\
		for (pos = dlist_first_entry(head, __typeof__(*pos), member),	\
			n = dlist_next_entry(pos, member); \
			 &pos->member != (head);					\
			 pos = n, n = dlist_next_entry(pos, member))


static inline void dlist_add(struct dlist *p_new,
				  struct dlist *prev,
				  struct dlist *next)
{
	next->prev = p_new;
	p_new->next = next;
	p_new->prev = prev;
	prev->next = p_new;
}

static inline void dlist_add_head(struct dlist *p_new, struct dlist *p_head)
{
	dlist_add(p_new, p_head, p_head->next);
}

static inline void dlist_add_tail(struct dlist *p_new, struct dlist *p_head)
{
	dlist_add(p_new, p_head->prev, p_head);
}

static inline void dlist_del(struct dlist *entry)
{
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
}

#define dlist_empty(head) ((head)->next == (head))

#ifdef __cplusplus
}
#endif

