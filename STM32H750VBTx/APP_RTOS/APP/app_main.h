#ifndef __APP_MAIN_H
#define __APP_MAIN_H

#include "main.h"
#include "cmsis_os.h"

/* Function prototypes */
void app_main_init(void);
void task_log_entry(void *argument);
void task_lcd_entry(void *argument);

#endif /* __APP_MAIN_H */
