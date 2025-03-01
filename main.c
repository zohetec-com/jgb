#include "debug.h"
#include <unistd.h>

extern int jgb_set_conf_dir(const char* d);
extern int jgb_install(const char* appname, const char* libfile);

int main(int argc, char *argv[])
{
    jgb_info("jgb start.");

    int c;

    while ((c = getopt (argc, argv, "D:")) != -1)
    {
        switch (c)
        {
        case 'D':
            jgb_set_conf_dir(optarg);
            break;
        default:
            break;
        }
    }

    jgb_install("xyz", NULL);

    return 0;
}
