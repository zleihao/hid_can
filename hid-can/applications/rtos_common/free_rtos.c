#include "RTOS.h"

/* ��Ҫ�޸����������� */
int task_index;
init_task_start_t task_create_func[USER_TASK_NUM];

void AppTaskCreate(void)
{
    uint8_t i;
    taskENTER_CRITICAL();

    /* ���δ������� */
    for (i = 0; i < USER_TASK_NUM; i++) {
        if (task_create_func[i] == NULL) {
            continue;
        }
        (*task_create_func[i])();
    }

    taskEXIT_CRITICAL();

    vTaskStartScheduler();
}
