#include "config.h"
#include "error.h"
#include "debug.h"

namespace jgb
{

value::~value()
{
    if(len_ > 0)
    {
        if(type_ == data_type::string)
        {
            for(int i=0; i<len_; i++)
            {
                free((void*)str_[i]);
            }
        }
        else if(type_ == data_type::object)
        {
            for(int i=0; i<len_; i++)
            {
                delete conf_[i];
            }
        }
        delete [] int_;
    }
}

}
