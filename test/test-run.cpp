#include <jgb/core.h>

static int init(void*)
{
    for(auto it = jgb::core::get_instance()->app_.begin(); it != jgb::core::get_instance()->app_.end(); ++it)
    {
        for(auto it2 = (*it)->instances_.begin(); it2 != (*it)->instances_.end(); ++it2)
        {
            (*it2)->start();
        }
    }
    return 0;
}

jgb_api_t test_run
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "start all instances",
    .init = init,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = nullptr
};
