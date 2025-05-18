#ifndef _CAN_PROCESS_H
#define _CAN_PROCESS_H

#include "stm32f4xx.h"

#define CAN_RX_PIN       GPIO_Pin_12
#define CAN_TX_PIN       GPIO_Pin_13
#define CAN_TX_GPIO_PORT GPIOB
#define CAN_RX_GPIO_PORT GPIOB

#define CAN_TX_GPIO_CLK RCC_AHB1Periph_GPIOB
#define CAN_RX_GPIO_CLK RCC_AHB1Periph_GPIOB

#define CAN_AF_PORT   GPIO_AF_CAN2
#define CAN_RX_SOURCE GPIO_PinSource12
#define CAN_TX_SOURCE GPIO_PinSource13

typedef struct {
    uint8_t can_state;  //can状态，打开 or 关闭

    /* 波特率相关 */
    uint8_t sjw;
    uint16_t psc;
    uint8_t bs1;
    uint8_t bs2;

    uint8_t work_mode;
    uint8_t frame_type;
    uint32_t frame_id;
    uint8_t data_type;
    uint8_t dlc;
    uint8_t time_stamp_flag;
    uint8_t recv_fifo_number;
    /* 滤波配置 */
    uint8_t filter_number;
    uint8_t filter_mode;
    uint8_t filter_scale;

    uint16_t filter_id_hight;
    uint16_t filter_id_low;

    uint16_t filter_mask_id_hight;
    uint16_t filter_mask_id_low;
} can_param_t;

extern can_param_t g_can_config;

typedef struct {
    uint32_t baud;
    uint8_t sjw;
    uint16_t psc;
    uint8_t bs1;
    uint8_t bs2;
} can_baud_t;

void can_init(void);

#endif