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

std::ostream& operator<<(std::ostream& os, const value* val)
{
    if(val->is_array_)
    {
        os << '[';
    }

    switch (val->type_) {
        case value::data_type::integer:
        {
            for(int i=0; i<val->len_; i++)
            {
                if(!val->is_bool_)
                {
                    os << val->int_[i];
                }
                else
                {
                    if(val->int_[i])
                    {
                        os << "true";
                    }
                    else
                    {
                        os << "false";
                    }
                }
                if(i+1 < val->len_)
                {
                    os << ',';
                }
            }
        }
        break;
        case value::data_type::real:
        {
            for(int i=0; i<val->len_; i++)
            {
                os << val->real_[i];
                if(i+1 < val->len_)
                {
                    os << ',';
                }
            }
        }
        break;
        case value::data_type::string:
        {
            for(int i=0; i<val->len_; i++)
            {
                os << '"' << val->str_[i] << '"';
                if(i+1 < val->len_)
                {
                    os << ',';
                }
            }
        }
        break;
        case value::data_type::object:
        {
            for(int i=0; i<val->len_; i++)
            {
                os << val->conf_[i];
                if(i+1 < val->len_)
                {
                    os << ',';
                }
            }
        }
        break;
    }

    if(val->is_array_)
    {
        os << ']';
    }

    return os;
}

std::ostream& operator<<(std::ostream& os, const pair* pr)
{
    os << '"' << pr->name_ << '"' << ": ";
    os << pr->value_;
    return os;
}

std::ostream& operator<<(std::ostream& os, const config* conf)
{
    os << '{';
    int n = conf->conf_.size();
    int nn = 0;
    for (auto & i : conf->conf_) {
        os << i;
        ++ nn;
        if(nn < n)
        {
            os << ',';
        }
    }
    os << '}';
    return os;
}

}
