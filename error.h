#ifndef ERROR_H_20250301
#define ERROR_H_20250301

#define JGB_OK                              0       // 成功、正常
#define JGB_FAIL                            1000    // 成功、正常
#define JGB_ERR_DENIED                      1001    // 请求本身没有问题，但条件所限，只能拒绝
#define JGB_ERR_INVALID                     1002    // 无效的请求：内容缺失、格式无效
#define JGB_ERR_NOT_SUPPORT                 1003    // 不支持所请求的操作：尚未实现，或者已经被移除

#endif // ERROR_H
