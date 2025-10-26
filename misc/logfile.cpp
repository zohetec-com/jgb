#include <jgb/core.h>
#include <jgb/buffer.h>
#include <jgb/helper.h>
#include <string>
#include <sys/stat.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/trim.hpp>

static std::string get_conf_name()
{
    std::string conf_dir = jgb::core::get_instance()->conf_dir();
    boost::algorithm::trim_right_if(conf_dir, boost::is_any_of("/"));
    if(conf_dir == ".")
    {
        conf_dir = boost::filesystem::current_path().string();
        fprintf(stderr, "{ cwd = %s }\n", conf_dir.c_str());
    }
    boost::filesystem::path p(conf_dir);
    return p.filename().string();
}

struct context_a01064074357
{
    std::string dir_;
    std::string name_;
    int count_;
    int size_;
    int level_;
    FILE* fp_;
    int written;

    context_a01064074357(jgb::config* conf)
        : dir_("/data/log.d"),
        count_(3),
        size_(2 * 1024 * 1024),
        level_(JGB_LOG_RAW),
        fp_(nullptr),
        written(0)
    {
        if(conf)
        {
            conf->get("dir", dir_);
            conf->get("name", name_);
            conf->get("count", count_);
            conf->get("size", size_);
            conf->get("level", level_);
        }
    }

    std::string get_filename()
    {
        char buf[256];
        int r;
        int off = 0;
        const char* p = name_.c_str();
        fprintf(stderr, "{ name = %s, conf name = %s }\n", name_.c_str(), get_conf_name().c_str());
        while(*p)
        {
            if(*p == '%')
            {
                ++ p;
                switch (*p)
                {
                case 'c':
                    {
                        r = jgb::put_string(buf, 256, off, "%s", get_conf_name().c_str());
                        jgb_assert(!r);
                        ++ p;
                        break;
                    }
                default:
                    if(*p != '\0')
                    {
                        ++ p;
                    }
                }
            }
            else
            {
                r = jgb::put_string(buf, 256, off, "%c", *p);
                jgb_assert(!r);
                ++ p;
            }
        }
        buf[off] = '\0';
        return std::string(buf);
    }

    std::string get_file_path()
    {
        return dir_ + '/' + get_filename();
    }

    void backup_file()
    {
        std::string src_file_path;
        struct stat st;
        for(int i=count_-1; i>0; i--)
        {
            src_file_path = get_file_path();
            if(i>1)
            {
                src_file_path += '.' + std::to_string(i-1);
            }
            if(!stat(src_file_path.c_str(), &st))
            {
                std::string dst_file_path = get_file_path() + '.' + std::to_string(i);
                int r = rename(src_file_path.c_str(), dst_file_path.c_str());
                fprintf(stderr, "rename %s => %s\n", src_file_path.c_str(), dst_file_path.c_str());
                if(r)
                {
                    jgb_assert(0);
                }
            }
        }
    }

    int open_file()
    {
        if(!fp_)
        {
            std::string filepath = get_file_path();
            fprintf(stderr, "{ filepath = %s }\n", filepath.c_str());
            backup_file();
            fp_ = fopen(filepath.c_str(), "w");
        }
        return fp_ ? 0 : JGB_ERR_FAIL;
    }

    void close_file()
    {
        if(fp_)
        {
            fclose(fp_);
            fp_ = nullptr;
            written = 0;
        }
    }

    int write(int level, char* buf, int len)
    {
        if(len + written > size_)
        {
            close_file();
        }
        if(!open_file())
        {
            if(level <= level_)
            {
                size_t n = fwrite(buf, 1, len, fp_);
                if(n != (size_t) len)
                {
                    return JGB_ERR_IO;
                }
            }
            return 0;
        }
        return JGB_ERR_FAIL;
    }

    ~context_a01064074357()
    {
        close_file();
    }
};

static int tsk_init(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    jgb::config* conf = w->get_config();
    context_a01064074357* ctx = new context_a01064074357(conf);
    w->set_user(ctx);
    jgb_debug("{ conf dir = %s, conf name = %s }",
              jgb::core::get_instance()->conf_dir(),
              get_conf_name().c_str());
    return 0;
}

static int tsk_loop(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_a01064074357* ctx = (context_a01064074357*) w->get_user();
    jgb::reader* rd = w->get_reader(0);
    jgb::frame frm;
    int r;
    r = rd->request_frame(&frm);
    if(!r)
    {
        log_frame_header* h = (log_frame_header*) frm.buf;
        r = ctx->write(h->level, h->log, frm.len - sizeof(log_frame_header));
        rd->release();
    }
    return 0;
}

static void tsk_exit(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    context_a01064074357* ctx = (context_a01064074357*) w->get_user();
    delete ctx;
}

static loop_ptr_t loops[] = { tsk_loop, nullptr };

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = loops,
    .exit = tsk_exit
};

jgb_api_t logfile
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "log to files",
    .init = nullptr,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
