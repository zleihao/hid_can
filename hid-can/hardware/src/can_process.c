#include "can_process.h"
#include "string.h"

can_param_t g_can_config;

/**
 * @brief  can引脚初始化
 * @param  null
 * @retval null
 */
static void can_gpio_config(void)
{
    memset(&g_can_config, 0x00, sizeof(g_can_config));

    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable GPIO clock */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    /* Connect CAN pins to AF9 */
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_CAN2);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_CAN2);

    /* Configure CAN TX pins */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* Configure CAN RX  pins */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

static void can_nvic_config(void)
{
    /*中断设置*/
    NVIC_InitTypeDef NVIC_InitStructure;
    NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX0_IRQn;        //CAN RX0中断
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 7;  //抢占优先级0
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;         //子优先级为0
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void can_init(void)
{
    can_gpio_config();
    can_nvic_config();
}
