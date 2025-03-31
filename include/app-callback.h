#ifndef APPCALLBACK_H_20250331
#define APPCALLBACK_H_20250331

typedef struct jgb_task_callback
{
    int (*init)(void* conf);
    int (**loop)(void* conf);
    void (*exit)(void* conf);
} jgb_task_callback_t;

typedef struct jgb_app_callback
{
    int (*init)(void* conf); // 初始化应用
    void (*release)(void* conf); // 释放应用
    int (*create)(void* conf);  // 创建实例
    void (*destroy)(void* conf); // 销毁实例
    int  (*commit)(void* conf); // 提交对参数的更改
    jgb_task_callback_t* task_callbak;
} jgb_app_callback_t;

#endif // APPCALLBACK_H
