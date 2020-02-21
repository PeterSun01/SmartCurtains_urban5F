#ifndef _MESH_H_
#define _MESH_H_

#include "mdf_common.h"
#include "mwifi.h"
#include "driver/uart.h"
#include "string.h"
#include "Smartconfig.h"
#include "Json_parse.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"


void Mesh_Init(char *wifi_ssid, char *wifi_password);
mdf_err_t Send_Mesh(uint32_t fire_level,char *data,size_t size,mwifi_data_type_t data_type);



/*
enum{
    FIRE = 0, //火情发生
    NOTH,     //火情结束
};
static uint8_t mesh_status = NOTH;
*/

enum{
    Send_message = 0,
    Receive_messages,
    Unknow_messages,
};

static uint8_t ToorRe_status = Unknow_messages; //该设备属于接收到修改信号或者发出信号设备

#endif

