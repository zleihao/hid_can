#include "RTOS.h"
#include "bsp_key.h"
#include "usb_config.h"
#include "delay_ms.h"
#include "can_process.h"

/************************************/
void usb_dc_low_level_init(uint8_t busid)
{
#ifdef USE_USB_FS
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_12;

    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_OTG1_FS);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource12, GPIO_AF_OTG1_FS);

    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_OTG_FS, ENABLE);

    //中断
    NVIC_InitTypeDef NVIC_InitStructure;

    // NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    NVIC_InitStructure.NVIC_IRQChannel = OTG_FS_IRQn;

    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 6;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#ifdef USB_OTG_HS_DEDICATED_EP1_ENABLED
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_EP1_OUT_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_EP1_IN_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif
#else
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 1. 启用相关 GPIO 时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

    /* 2. 启用 USB_OTG_HS 和 ULPI 时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_OTG_HS | RCC_AHB1Periph_OTG_HS_ULPI, ENABLE);

    /* 通用配置 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;

    /* GPIOA: PA3 (DATA0), PA5 (CLKOUT) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_OTG_HS);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_OTG_HS);

    /* GPIOB: PB0, PB1, PB5, PB10~PB13 (DATA1~DATA7) */
    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_5 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 | GPIO_Pin_13;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource0, GPIO_AF_OTG_HS);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource1, GPIO_AF_OTG_HS);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_OTG_HS);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource10, GPIO_AF_OTG_HS);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource11, GPIO_AF_OTG_HS);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource12, GPIO_AF_OTG_HS);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_OTG_HS);

    /* GPIOC: PC0 (STP), PC2 (DIR), PC3 (NXT) */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource0, GPIO_AF_OTG_HS);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource2, GPIO_AF_OTG_HS);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource3, GPIO_AF_OTG_HS);

    NVIC_InitTypeDef NVIC_InitStructure;

    /* 设置中断优先级（根据实际系统配置） */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    /* 配置 USB OTG HS 中断通道 */
    NVIC_InitStructure.NVIC_IRQChannel = OTG_HS_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif
}

void usb_dc_low_level_deinit(uint8_t busid)
{
    //todo
}

void usbd_dwc2_delay_ms(uint8_t ms)
{
    delay_ms(ms);
}

uint32_t usbd_get_dwc2_gccfg_conf(uint32_t reg_base)
{
#ifdef CONFIG_USB_HS
    return 0;
#else
    return ((1 << 16) | (1 << 18) | (1 << 19) | (1 << 21));
#endif
}

int kprintf(const char *fmt, ...)
{
    /* todo */
    return 0;
}

extern int led_init(void);
void bsp_init(void)
{
    // NVIC_SetPriorityGrouping(NVIC_PriorityGroup_4);
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);

    led_init();
    key_init();
    timer_init();

    // can_config();
    can_init();
}

/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
int main(void)
{
    bsp_init();

    AppTaskCreate();

    while (1) {
        ;
    }
}

#ifdef USE_USB_FS
void OTG_FS_IRQHandler(void)
{
    extern void USBD_IRQHandler(uint8_t busid);
    USBD_IRQHandler(0);
}
#else
void OTG_HS_IRQHandler(void)
{
    extern void USBD_IRQHandler(uint8_t busid);
    USBD_IRQHandler(0);
}
#endif

/*********************************************END OF FILE**********************/
