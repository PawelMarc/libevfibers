/********************************************************************

  Copyright 2012 Konstantin Olkhovskiy <lupus@oxnull.net>

  This file is part of libevfibers.

  libevfibers is free software: you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or any later version.

  libevfibers is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with libevfibers.  If not, see
  <http://www.gnu.org/licenses/>.

 ********************************************************************/

#ifndef _FBR_FIBER_PRIVATE_H_
#define _FBR_FIBER_PRIVATE_H_

#include <stdarg.h>
#include <stdint.h>
#include <sys/queue.h>
#include <evfibers/fiber.h>
#include <evfibers_private/trace.h>
#include <evfibers_private/kvec.h>
#include <coro.h>
#include <vrb.h>

#define obstack_chunk_alloc malloc
#define obstack_chunk_free free

#define FBR_CALL_LIST_WARN 1000

#define max(a,b) ({						\
		const typeof(a) __tmp_a = (a);			\
		const typeof(b) __tmp_b = (b);			\
		__tmp_a > __tmp_b ? __tmp_a : __tmp_b;		\
		})

#define _unused_ __attribute__((unused))

struct mem_pool {
	void *ptr;
	fbr_alloc_destructor_func_t destructor;
	void *destructor_context;
	LIST_ENTRY(mem_pool) entries;
};

LIST_HEAD(mem_pool_list, mem_pool);

TAILQ_HEAD(fiber_destructor_tailq, fbr_destructor);
LIST_HEAD(fiber_list, fbr_fiber);

struct fbr_fiber {
	uint32_t id;
	uint32_t fibers_index;
	char name[FBR_MAX_FIBER_NAME];
	fbr_fiber_func_t func;
	void *func_arg;
	coro_context ctx;
	char *stack;
	size_t stack_size;
	struct {
		struct fbr_ev_base **waiting;
		int arrived;
	} ev;
	struct trace_info reclaim_tinfo;
	struct fiber_list children;
	struct fbr_fiber *parent;
	struct mem_pool_list pool;
	struct {
		LIST_ENTRY(fbr_fiber) reclaimed;
		LIST_ENTRY(fbr_fiber) children;
		LIST_ENTRY(fbr_fiber) transfer_pending;
	} entries;
	struct fiber_destructor_tailq destructors;
	void *user_data;
	void *key_data[FBR_MAX_KEY];
	int no_reclaim;
	int want_reclaim;
	struct fbr_cond_var reclaim_cond;
	struct {
		int enabled;
		int flags;
	} foreign;
};

TAILQ_HEAD(mutex_tailq, fbr_mutex);

struct fbr_async {
	pid_t worker_pid;
	int read_fd, write_fd;
	struct fbr_mutex mutex;
	void *buf;
	size_t buf_size;
	SLIST_ENTRY(fbr_async) entries;
};

SLIST_HEAD(fbr_async_slist, fbr_async);

struct fbr_stack_item {
	struct fbr_fiber *fiber;
	struct trace_info tinfo;
};

struct fbr_context_private {
	struct fbr_stack_item stack[FBR_CALL_STACK_SIZE];
	struct fbr_stack_item *sp;
	struct fbr_fiber root;
	struct fiber_list reclaimed;
	struct ev_async pending_async;
	struct ev_async loop_wakeup_async;
	struct fbr_id_tailq pending_fibers;
	int backtraces_enabled;
	uint32_t last_id;
	uint64_t key_free_mask;
	struct fbr_async_slist free_workers;
	struct {
		kvec_t(struct fbr_fiber *) fibers;
		uint32_t new_id;
	} ids;
	struct {
		kvec_t(fbr_id_t) transfer_pending;
		kvec_t(fbr_id_t) transfer_pending_copy;
	} foreign;

	struct ev_loop *loop;
};

int fbr_async_debug(FBR_P_ struct fbr_async *async);

#endif
