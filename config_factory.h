#ifndef CONFIG_FACTORY_H
#define CONFIG_FACTORY_H

#include "config.h"

namespace jgb
{

class config_factory
{
public:
    static config* create(const char* buf, int len);
    static config* create(const char* file_path);
};

} // namespace jgb

#endif // CONFIG_FACTORY_H
