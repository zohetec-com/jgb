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
#include "log.h"
#include "helper.h"
#include <unistd.h>
#include "core.h"
#include <signal.h>

extern jgb_api_t module;
extern jgb_api_t logbuf;
extern int jgb_log_print_level;
static bool exit_flag = false;

// 处理 SIGINT 信号
static void handler(int signum)
{
    jgb_notice("signal catched. { signum = %d }", signum);
    if(signum == SIGINT)
    {
        exit_flag = true;

        static int count = 0;
        ++ count;
        if(count > 10)
        {
            exit(1);
        }
    }
}

int main(int argc, char *argv[])
{
    jgb_info("jgb start.");

    int c;
    while ((c = getopt (argc, argv, "D:v:")) != -1)
    {
        switch (c)
        {
        case 'D':
            jgb::core::get_instance()->set_conf_dir(optarg);
            break;
        case 'v':
            jgb::stoi(optarg, jgb_log_print_level);
            break;
        default:
            break;
        }
    }

    jgb::core::get_instance()->install("logbuf", &logbuf);

    // 注册 SIGINT 信号处理函数
    struct sigaction act = {};

    // https://man7.org/linux/man-pages/man7/signal.7.html
    act.sa_handler = &handler;
    if (sigaction(SIGINT, &act, NULL) == -1)
    {
        perror("sigaction");
    }
    if (sigaction(SIGUSR1, &act, NULL) == -1)
    {
        perror("sigaction");
    }

    jgb::core::get_instance()->install("module", &module);

    while(!exit_flag)
    {
        sleep(1);
    }

    jgb::core::get_instance()->uninstall_all();

    return 0;
}
