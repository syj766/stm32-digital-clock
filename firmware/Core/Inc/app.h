/**
 * @file    app.h
 * @brief   应用层：模块装配 + 时钟主逻辑
 */
#ifndef APP_H
#define APP_H

#ifdef __cplusplus
extern "C" {
#endif

void app_init(void);   /* 挂在 main() USER CODE BEGIN 2 */
void app_task(void);   /* 挂在 while(1) USER CODE BEGIN 3 */

#ifdef __cplusplus
}
#endif

#endif /* APP_H */