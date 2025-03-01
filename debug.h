#ifndef DEBUG_H_20250301
#define DEBUG_H_20250301

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#define jgb_debug(fmt, ...)     fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_info(fmt, ...)      fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_notice(fmt, ...)    fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_warning(fmt, ...)   fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_error(fmt, ...)     fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_fatal(fmt, ...)     fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_bug(fmt, ...)       fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define jgb_assert(x)           assert(x)

#endif // DEBUG_H
