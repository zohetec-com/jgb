#include "error.h"
#include "debug.h"
#include "helper.h"

// 输入不能包含有空格！
int get_dir(const char** start, const char** end)
{
    if(start && *start)
    {
        const char* p = *start;
        while(*p == '/' && *p != '\0')
        {
            ++ p;
        }
        *start = p;
        if(end)
        {
            jgb_assert(*p != '/');
            while(*p != '/' && *p != '\0')
            {
                ++ p;
            }
            *end = p;
        }
        return 0;
    }
    else
    {
        return JGB_ERR_INVALID;
    }
}
