#ifndef DEBUG_H_20250301
#define DEBUG_H_20250301

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#define jgb_debug(fmt, ...)     fprintf(stderr, "[%s:%d][DEBUG] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_info(fmt, ...)      fprintf(stderr, "[%s:%d][INFO] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_notice(fmt, ...)    fprintf(stderr, "[%s:%d][NOTICE] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_warning(fmt, ...)   fprintf(stderr, "[%s:%d][WARNING] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_error(fmt, ...)     fprintf(stderr, "[%s:%d][ERROR] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
// 错误的特例
#define jgb_bug(fmt, ...)       fprintf(stderr, "[%s:%d][BUG] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_fail(fmt, ...)      fprintf(stderr, "[%s:%d][FAIL] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define jgb_fatal(fmt, ...)     fprintf(stderr, "[%s:%d][FATAL] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define jgb_assert(x)           assert(x)

#define jgb_safe_delete_object(p)       do{ delete p; p=NULL; } while(0)
#define jgb_safe_delete_array(p)        do{ delete[] p; p=NULL; } while(0)
#define jgb_safe_free(p)                do{ if(p){ free(p); p=NULL; } } while(0)

#endif // DEBUG_H
