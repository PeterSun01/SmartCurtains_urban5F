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
#include <cJSON.h>

void Mesh_Init(char *wifi_ssid, char *wifi_password);

mdf_err_t Send_Fire_Mesh(uint32_t fire_level);
mdf_err_t Semd_DeviceInfo_Mesh();//productID SerialNum ChannelId APIKey 

mdf_err_t parse_noderead_mesh(char *nodedata);
mdf_err_t parse_rootread_mesh(char *rootdata);


enum{
    FIRE = 0, //火情发生
    NOTH,     //火情结束
};
static uint8_t mesh_status = NOTH;

enum{
    Send_Device = 0,
    Receive_Device,
    Default_Device,
};
static uint8_t Device_status = Default_Device; //该设备属于接收到修改信号或者发出信号设备

enum{
    PARSE_OK = 0,
    PARSE_ERR = 1,
};

static struct MESH_STA{
    char FIRESTA[8];
    char PROJSTA[8]; 
    char WARNING[8]; //火灾警报
    char NOTHING[8]; //火灾解除
}MEST = {
            "FIRESTA",
            "PROJSTA",
            "WARNING",
            "NOTHING",
        };

static const uint32_t io_num = 27;// 引脚号
#endif

