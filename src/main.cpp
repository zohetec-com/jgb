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
#include "debug.h"
#include <unistd.h>
#include "core.h"

int main(int argc, char *argv[])
{
    jgb_info("jgb start.");

    int c;

    while ((c = getopt (argc, argv, "D:")) != -1)
    {
        switch (c)
        {
        case 'D':
            jgb::core::get_instance()->set_conf_dir(optarg);
            break;
        default:
            break;
        }
    }

    for(int i=0; i<3; i++)
    {
        jgb_debug("round %d", i + 1);
        jgb::core::get_instance()->install("test_core", "libtest_core.so");
        jgb::core::get_instance()->start("test_core");
        sleep(1);
        jgb::core::get_instance()->uninstall_all();
    }

    return 0;
}
