#include "stm32f4xx.h"
#include "can_process.h"
#include "string.h"
#include "usbd_core.h"
#include "FreeRTOS.h"
#include "timers.h"

can_baud_t can_baud[5] = {
    { .baud = 1000, .sjw = CAN_SJW_1tq, .psc = 6, .bs1 = CAN_BS1_4tq, .bs2 = CAN_BS2_2tq },
    { .baud = 500, .sjw = CAN_SJW_1tq, .psc = 12, .bs1 = CAN_BS1_4tq, .bs2 = CAN_BS2_2tq },
    { .baud = 250, .sjw = CAN_SJW_1tq, .psc = 24, .bs1 = CAN_BS1_4tq, .bs2 = CAN_BS2_2tq },
    { .baud = 125, .sjw = CAN_SJW_1tq, .psc = 48, .bs1 = CAN_BS1_4tq, .bs2 = CAN_BS2_2tq },
    { .baud = 100, .sjw = CAN_SJW_1tq, .psc = 60, .bs1 = CAN_BS1_4tq, .bs2 = CAN_BS2_2tq },
};

CanRxMsg RxMessage;  //接收缓冲区
CanTxMsg TxMessage;  //发送缓冲区

//配置设备模式
static void can_mode_config()
{
    //设备已经打开了
    CAN_InitTypeDef CAN_InitStructure;

    //关闭设备
    CAN_ITConfig(CAN2, CAN_IT_FMP0, DISABLE);

    /************************CAN通信参数设置**********************************/
    /*CAN寄存器初始化*/
    CAN_DeInit(CAN2);
    CAN_StructInit(&CAN_InitStructure);

    /*CAN单元初始化*/
    if (1 == g_can_config.time_stamp_flag) {
        CAN_InitStructure.CAN_TTCM = ENABLE;  //MCR-TTCM  开启时间触发通信模式使能
    } else {
        CAN_InitStructure.CAN_TTCM = DISABLE;  //MCR-TTCM  关闭时间触发通信模式使能
    }
    CAN_InitStructure.CAN_ABOM = ENABLE;                  //MCR-ABOM  自动离线管理
    CAN_InitStructure.CAN_AWUM = ENABLE;                  //MCR-AWUM  使用自动唤醒模式
    CAN_InitStructure.CAN_NART = DISABLE;                 //MCR-NART  禁止报文自动重传	  DISABLE-自动重传
    CAN_InitStructure.CAN_RFLM = DISABLE;                 //MCR-RFLM  接收FIFO 锁定模式  DISABLE-溢出时新报文会覆盖原有报文
    CAN_InitStructure.CAN_TXFP = DISABLE;                 //MCR-TXFP  发送FIFO优先级 DISABLE-优先级取决于报文标示符
    CAN_InitStructure.CAN_Mode = g_can_config.work_mode;  //正常工作模式
    CAN_InitStructure.CAN_SJW = g_can_config.sjw;         //BTR-SJW 重新同步跳跃宽度 2个时间单元

    /* ss=1 bs1=4 bs2=2 位时间宽度为(1+4+2) 波特率即为时钟周期tq*(1+4+2)  */
    CAN_InitStructure.CAN_BS1 = g_can_config.bs1;  //BTR-TS1 时间段1 占用了4个时间单元
    CAN_InitStructure.CAN_BS2 = g_can_config.bs2;  //BTR-TS1 时间段2 占用了2个时间单元

    /* CAN Baudrate = 1 MBps (1MBps已为stm32的CAN最高速率) (CAN 时钟频率为 APB 1 = 42 MHz) */
    CAN_InitStructure.CAN_Prescaler = g_can_config.psc;  //BTR-BRP 波特率分频器  定义了时间单元的时间长度 42/(1+4+2)/6=1 Mbps
    CAN_Init(CAN2, &CAN_InitStructure);

    //打开设备
    CAN_ITConfig(CAN2, CAN_IT_FMP0, ENABLE);
}

/**
 * @brief 配置设备滤波
 */
static void can_filter_config(void)
{
    /* 设置 can 滤波 */
    CAN_FilterInitTypeDef CAN_FilterInitStructure;

    /*CAN筛选器初始化*/
    CAN_FilterInitStructure.CAN_FilterNumber = g_can_config.filter_number;  //筛选器组14
    CAN_FilterInitStructure.CAN_FilterMode = g_can_config.filter_mode;      //工作在掩码模式
    CAN_FilterInitStructure.CAN_FilterScale = g_can_config.filter_scale;    //筛选器位宽为单个32位。
                                                                            /* 使能筛选器，按照标志的内容进行比对筛选，扩展ID不是如下的就抛弃掉，是的话，会存入FIFO0。 */

    CAN_FilterInitStructure.CAN_FilterIdHigh = g_can_config.filter_id_hight;           //要筛选的ID高位
    CAN_FilterInitStructure.CAN_FilterIdLow = g_can_config.filter_id_low;              //要筛选的ID低位
    CAN_FilterInitStructure.CAN_FilterMaskIdHigh = g_can_config.filter_mask_id_hight;  //筛选器高16位每位必须匹配
    CAN_FilterInitStructure.CAN_FilterMaskIdLow = g_can_config.filter_mask_id_low;     //筛选器低16位每位必须匹配

    CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_Filter_FIFO0;  //筛选器被关联到FIFO0
    CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;                //使能筛选器
    CAN_FilterInit(&CAN_FilterInitStructure);

    /*CAN通信中断使能*/
    CAN_ITConfig(CAN2, CAN_IT_FMP0, ENABLE);
}

//=======================按照指令进行操作===============================
static void set_can_baud(const uint8_t *request)
{
    uint32_t baud = 0;
    uint8_t i = 0;

    memcpy(&baud, request, 4);
    if (0x00 == baud || 0xFFFFFFFF == baud) {
        //使用自定义波特率
        g_can_config.sjw = request[4];
        memcpy(&g_can_config.psc, &request[5], 2);
        g_can_config.bs1 = request[7];
        g_can_config.bs2 = request[8];

        return;
    }

    //直接使用默认的
    for (i = 0; i < (sizeof(can_baud) / sizeof(can_baud_t)); i++) {
        if (baud == can_baud[i].baud) {
            g_can_config.sjw = can_baud[i].sjw;
            g_can_config.psc = can_baud[i].psc;
            g_can_config.bs1 = can_baud[i].bs1;
            g_can_config.bs2 = can_baud[i].bs2;

            return;
        }
    }

    //传输的数据不满足，清0
    g_can_config.sjw = 0;
    g_can_config.psc = 0;
    g_can_config.bs1 = 0;
    g_can_config.bs2 = 0;

    //判断设备是打开状态还是关闭状态
    if (g_can_config.can_state) {
        can_mode_config();
    }
}

static void set_work_mode(const uint8_t *request)
{
    g_can_config.work_mode = request[0];

    if (g_can_config.can_state) {
        can_mode_config();
    }
}

static void set_frame_type(const uint8_t *request)
{
    g_can_config.frame_type = request[0];

    memcpy(&g_can_config.frame_id, &request[1], 4);
}

static void set_data_type(const uint8_t *request)
{
    g_can_config.data_type = request[0];
}

static void set_data_length(const uint8_t *request)
{
    g_can_config.dlc = request[0];
}

static void set_time_stamp(const uint8_t *request)
{
    g_can_config.time_stamp_flag = request[0];
}

static void set_fifo_num(const uint8_t *request)
{
    g_can_config.recv_fifo_number = request[0];
}

static void set_filter(const uint8_t *request)
{
    g_can_config.filter_number = request[0];

    if (1 == request[1]) {
        g_can_config.filter_mode = CAN_FilterMode_IdList;
    } else {
        g_can_config.filter_mode = CAN_FilterMode_IdMask;
    }

    if (1 == request[2]) {
        g_can_config.filter_scale = CAN_FilterScale_32bit;
    } else {
        g_can_config.filter_scale = CAN_FilterScale_16bit;
    }

    memcpy(&g_can_config.filter_id_hight, &request[3], 2);
    memcpy(&g_can_config.filter_id_low, &request[5], 2);

    memcpy(&g_can_config.filter_mask_id_hight, &request[7], 2);
    memcpy(&g_can_config.filter_mask_id_low, &request[9], 2);

    if (g_can_config.can_state) {
        can_filter_config();
    }
}

static void open_can(const uint8_t *request)
{
    if (request[0] == 0) {
        g_can_config.can_state = 1;  //打开设备

        CAN_InitTypeDef CAN_InitStructure;
        /************************CAN通信参数设置**********************************/
        /* Enable CAN clock */
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1 | RCC_APB1Periph_CAN2, ENABLE);

        /*CAN寄存器初始化*/
        CAN_DeInit(CAN2);
        CAN_StructInit(&CAN_InitStructure);

        /*CAN单元初始化*/
        CAN_InitStructure.CAN_TTCM = DISABLE;                 //MCR-TTCM  关闭时间触发通信模式使能
        CAN_InitStructure.CAN_ABOM = ENABLE;                  //MCR-ABOM  自动离线管理
        CAN_InitStructure.CAN_AWUM = ENABLE;                  //MCR-AWUM  使用自动唤醒模式
        CAN_InitStructure.CAN_NART = DISABLE;                 //MCR-NART  禁止报文自动重传	  DISABLE-自动重传
        CAN_InitStructure.CAN_RFLM = DISABLE;                 //MCR-RFLM  接收FIFO 锁定模式  DISABLE-溢出时新报文会覆盖原有报文
        CAN_InitStructure.CAN_TXFP = DISABLE;                 //MCR-TXFP  发送FIFO优先级 DISABLE-优先级取决于报文标示符
        CAN_InitStructure.CAN_Mode = g_can_config.work_mode;  //正常工作模式
        CAN_InitStructure.CAN_SJW = g_can_config.sjw;         //BTR-SJW 重新同步跳跃宽度 2个时间单元

        /* ss=1 bs1=4 bs2=2 位时间宽度为(1+4+2) 波特率即为时钟周期tq*(1+4+2)  */
        CAN_InitStructure.CAN_BS1 = g_can_config.bs1;  //BTR-TS1 时间段1 占用了4个时间单元
        CAN_InitStructure.CAN_BS2 = g_can_config.bs2;  //BTR-TS1 时间段2 占用了2个时间单元

        /* CAN Baudrate = 1 MBps (1MBps已为stm32的CAN最高速率) (CAN 时钟频率为 APB 1 = 42 MHz) */
        CAN_InitStructure.CAN_Prescaler = g_can_config.psc;  //BTR-BRP 波特率分频器  定义了时间单元的时间长度 42/(1+4+2)/6=1 Mbps
        CAN_Init(CAN2, &CAN_InitStructure);

        can_filter_config();
    } else {
        g_can_config.can_state = 0;  //关闭状态

        //关闭设备
        CAN_ITConfig(CAN2, CAN_IT_FMP0, DISABLE);
    }
}

static void send_data(const uint8_t *request)
{
    uint8_t size = request[4];

    memcpy(&g_can_config.frame_id, &request[0], 4);

    /* 数据类型 */
    if (1 == g_can_config.data_type) {
        //远程帧
        TxMessage.RTR = CAN_RTR_REMOTE;
    } else {
        //数据帧
        TxMessage.RTR = CAN_RTR_DATA;
    }

    /* 帧类型 */
    if (1 == g_can_config.frame_type) {
        //扩展模式
        TxMessage.IDE = CAN_ID_EXT;
        //使用的扩展ID
        TxMessage.ExtId = g_can_config.frame_id;
    } else {
        //标准模式
        TxMessage.IDE = CAN_ID_STD;
        //使用的标准ID
        TxMessage.StdId = g_can_config.frame_id;
    }

    TxMessage.DLC = g_can_config.dlc;  //数据长度为8字节
    memcpy(TxMessage.Data, &request[5], size);

    CAN_Transmit(CAN2, &TxMessage);
}

void HID_CAN_ExecuteCommand(const uint8_t *request)
{
    switch (request[0]) {
    case 0x01:
        //设置波特率
        set_can_baud(&request[1]);
        break;

    case 0x02:
        //设置工作模式
        set_work_mode(&request[1]);
        break;

    case 0x03:
        //设置帧类型
        set_frame_type(&request[1]);
        break;

    case 0x04:
        //设置数据类型
        set_data_type(&request[1]);
        break;

    case 0x05:
        //设置数据长度
        set_data_length(&request[1]);
        break;

    case 0x06:
        //时间戳功能
        set_time_stamp(&request[1]);
        break;

    case 0x07:
        //接收 FIFO 选择
        set_fifo_num(&request[1]);
        break;

    case 0x08:
        //设置滤波功能
        set_filter(&request[1]);
        break;

    case 0x09:
        //发送数据
        send_data(&request[1]);
        break;

    case 0x0A:
        //打开设备
        open_can(&request[1]);
        break;
    }
}

extern uint8_t send_buffer[64];
extern uint8_t custom_state;

/**
 * @brief 中断下半部，专门用来把收到的数据发送给主机
 */
void can_irq(void *parameter1, uint32_t ulParameter2)
{
    (void)parameter1;
    (void)ulParameter2;

    memset(send_buffer, 0x00, 64);
    send_buffer[0] = 0x02;
    memset(&send_buffer[1], 0x00, 4);

    if (RxMessage.IDE == CAN_Id_Standard) {
        send_buffer[5] = 0;
        memcpy(&send_buffer[6], &RxMessage.StdId, 4);
    } else {
        send_buffer[5] = 1;
        memcpy(&send_buffer[6], &RxMessage.ExtId, 4);
    }

    if (CAN_RTR_Data == RxMessage.RTR) {
        send_buffer[10] = 0;
    } else {
        send_buffer[10] = 1;
    }

    send_buffer[11] = RxMessage.DLC;

    memcpy(&send_buffer[12], RxMessage.Data, RxMessage.DLC);

    usbd_ep_start_write(0, 0x81, send_buffer, 64);

    custom_state = 1;
    while (custom_state == 1) {
        ;
    }
}

void CAN2_RX0_IRQHandler(void)
{
    BaseType_t hight_task_woken = pdFALSE;
    BaseType_t ret = pdFALSE;

    /*从邮箱中读出报文*/
    CAN_Receive(CAN2, CAN_FIFO0, &RxMessage);

    //唤醒中断下半部
    ret = xTimerPendFunctionCallFromISR(can_irq,  //中断下半部
                                        NULL,     //pvParameter1
                                        0,        //pvParameter2
                                        &hight_task_woken);

    if (pdPASS == ret) {
        portYIELD_FROM_ISR(hight_task_woken);
    }
}