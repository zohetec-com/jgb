/*
 * jgb - a framework for linux media streaming application
 *
 * Copyright (C) 2025 Beijing Zohetec Co., Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <jgb/log.h>
#include <jgb/app.h>
#include <jgb/config.h>
#include <sys/types.h>

static bool tsk_init_called = false;
static uint tsk_loop0_count = 0;
static uint tsk_loop1_count = 0;
static bool tsk_exit_called = false;
static bool create_called = false;
static bool destroy_called = false;

static int tsk_init(void*)
{
    jgb_function();
    jgb_assert(!tsk_init_called);
    jgb_assert(!tsk_loop0_count);
    jgb_assert(!tsk_loop1_count);
    jgb_assert(!tsk_exit_called);
    tsk_init_called = true;
    return 0;
}

static int tsk_loop0(void*)
{
    jgb_assert(tsk_init_called);
    jgb_assert(!tsk_exit_called);
    ++ tsk_loop0_count;
    return 0;
}

static int tsk_loop1(void*)
{
    jgb_assert(tsk_init_called);
    jgb_assert(!tsk_exit_called);
    ++ tsk_loop1_count;
    return 0;
}

static void tsk_exit(void*)
{
    jgb_assert(tsk_init_called);
    jgb_assert(!tsk_exit_called);
    jgb_assert(tsk_loop0_count);
    jgb_assert(tsk_loop1_count);
    jgb_debug("{ tsk_loop0_count = %u , tsk_loop0_count = %u }",
              tsk_loop0_count, tsk_loop1_count);
    tsk_exit_called = true;
}

static int init(void* conf)
{
    jgb::config* c = (jgb::config*) conf;
    int r;
    int ival;

    jgb_function();

    jgb_assert(!tsk_init_called);
    jgb_assert(!tsk_loop0_count);
    jgb_assert(!tsk_loop1_count);
    jgb_assert(!tsk_exit_called);

    r = c->get("p0", ival);
    jgb_assert(!r);
    jgb_assert(ival == 250402);

    return 0;
}

static void release(void* conf)
{
    jgb::config* c = (jgb::config*) conf;
    int r;
    int ival;

    jgb_function();

    jgb_assert(tsk_init_called);
    jgb_assert(tsk_exit_called);
    jgb_assert(tsk_loop0_count);
    jgb_assert(tsk_loop1_count);
    jgb_assert(create_called);
    jgb_assert(destroy_called);

    r = c->get("p0", ival);
    jgb_assert(!r);
    jgb_assert(ival == 250402);

    // reset
    tsk_init_called = false;
    tsk_loop0_count = 0;
    tsk_loop1_count = 0;
    tsk_exit_called = false;
    create_called = false;
    destroy_called = false;
}

static int create(void*)
{
    jgb_assert(!create_called);
    jgb_assert(!destroy_called);
    create_called = true;
    return 0;
}

static void destroy(void*)
{
    jgb_assert(create_called);
    jgb_assert(!destroy_called);
    destroy_called = true;
}

static loop_ptr_t loops[] = { tsk_loop0, tsk_loop1, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t test_core
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "test core",
    .init = init,
    .release = release,
    .create = create,
    .destroy = destroy,
    .commit = nullptr,
    .loop = &loop
};
