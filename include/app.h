#ifndef APPCALLBACK_H_20250331
#define APPCALLBACK_H_20250331

typedef int(*loop_ptr_t)(void*);

typedef struct jgb_task
{
    int (*setup)(void* worker);
    int (**loop)(void* worker);
    void (*exit)(void* worker);
} jgb_task_t;

#define MAKE_API_VERSION(major,minor)   ((major<<8)|(minor & 0xFF))
#define CURRENT_API_VERSION()           MAKE_API_VERSION(0, 1)

typedef struct jgb_api
{
    int version;
    const char* desc;
    int (*init)(void* conf); // 初始化应用；conf 是应用的配置
    void (*release)(void* conf); // 释放应用；conf 是应用的配置
    int (*create)(void* conf);  // 创建实例；conf 是实例的配置
    void (*destroy)(void* conf); // 销毁实例；conf 是实例的配置
    int  (*commit)(void* conf); // 使已更改的设置生效；conf 是实例的配置
    jgb_task_t* task;
} jgb_api_t;

#endif // APPCALLBACK_H
