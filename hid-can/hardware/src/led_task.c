#include "RTOS.h"
#include "stm32f4xx.h"

int led_init(void)
{
    /*����һ��GPIO_InitTypeDef���͵Ľṹ��*/
    GPIO_InitTypeDef GPIO_InitStructure;

    /*����LED��ص�GPIO����ʱ��*/
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    /*ѡ��Ҫ���Ƶ�GPIO����*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;

    /*��������ģʽΪ���ģʽ*/
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;

    /*�������ŵ��������Ϊ�������*/
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;

    /*��������Ϊ����ģʽ*/
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;

    /*������������Ϊ2MHz */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    /*���ÿ⺯����ʹ���������õ�GPIO_InitStructure��ʼ��GPIO*/
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    return 0;
}

static StaticTask_t led1_task_tcb;

static void led1_task(void *param)
{
    for (;;) {
        GPIO_ResetBits(GPIOB, GPIO_Pin_2);
        vTaskDelay(500);

        GPIO_SetBits(GPIOB, GPIO_Pin_2);
        vTaskDelay(500);
    }
}

void led_task_start()
{
    xTaskCreate(led1_task, "led1_task", 128, NULL, 2, (TaskHandle_t *)&led1_task_tcb);
}
INIT_APP_TASK(led_task_start);
