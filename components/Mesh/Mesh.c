
#include "Mesh.h"

static const char *TAG = "MineMesh";
wifi_sta_list_t wifi_sta_list   = {0x0};


uint32_t io_num = 27;// 引脚号

void root_read_task(void *arg)
{
    //mdf_err_t ret = MDF_OK;
    char *data    = MDF_CALLOC(1, MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0x0};
    cJSON *json_data_parse = NULL;

    MDF_LOGI("root_read_task is running");

    for(;;) {
        if (!esp_mesh_is_root()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            esp_wifi_ap_get_sta_list(&wifi_sta_list);
            //MDF_LOGI("root_read_task loop goto");
            continue;
        }
        //MDF_LOGI("root_read_task is start");
        memset(data, '\0', MWIFI_PAYLOAD_LEN);
        //ret = 
        mwifi_root_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        json_data_parse = cJSON_Parse(data);
        json_data_parse = cJSON_GetObjectItem(json_data_parse, "data");
        if (!strcmp(json_data_parse->valuestring, "WARRING")){
            if(ToorRe_status!=Send_message)
            {
                ToorRe_status = Receive_messages; //该设备接收到火灾信息才被改变 属于接受设备
                gpio_set_level(io_num,0);
                MDF_LOGI("root read FIRE !!!!!!!!!!!!!!!!!!!!!");
            }
        }else if (!strcmp(json_data_parse->valuestring, "NOTHING")){
            MDF_LOGI("root read NOTH !!!!!!!!!!!!!!!!!!!!!"); 
            gpio_set_level(io_num,1);
            ToorRe_status = Unknow_messages; //该设备收到灭火信号 恢复默认身份
        }
    cJSON_Delete(json_data_parse);
    }
    
    MDF_FREE(data);
    vTaskDelete(NULL);
}



static void node_read_task(void *arg)
{
    //mdf_err_t ret                    = MDF_OK;
    char *data                       = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size                      = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};

    cJSON *json_data_parse = NULL;
    cJSON *json_data_parse_user = NULL;
    cJSON *json_data_parse_data = NULL;
    MDF_LOGI("Note read task is running");

    for (;;) {
        if (!mwifi_is_connected()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        size = MWIFI_PAYLOAD_LEN;
        memset(data, 0, MWIFI_PAYLOAD_LEN);
        //ret =   
        mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        json_data_parse = cJSON_Parse(data);
        json_data_parse_user = cJSON_GetObjectItem(json_data_parse, "user");
        json_data_parse_data = cJSON_GetObjectItem(json_data_parse, "data");

        if (!strcmp(json_data_parse_data->valuestring, "WARRING")){
            if(ToorRe_status!=Send_message)
            {
                ToorRe_status = Receive_messages;
                MDF_LOGI("NODE FIRE!!!!!!!!!!!!!!!!!!!!!");
                gpio_set_level(io_num,0);
            }
        }else if(!strcmp(json_data_parse_data->valuestring, "NOTHING")){
            ToorRe_status = Unknow_messages;
            MDF_LOGI("NODE NO FIRE!!!!!!!!!!!!!!!!!!!!!");
            gpio_set_level(io_num,1);
        }

        MDF_LOGD("Node receive: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);
    }
    cJSON_Delete(json_data_parse);
    cJSON_Delete(json_data_parse_user);
    cJSON_Delete(json_data_parse_data);
    MDF_LOGW("Note read task is exit");

    MDF_FREE(data);
    vTaskDelete(NULL);
}


void root_write_task(void *arg)
{
    //mdf_err_t ret = MDF_OK;
    char *data    = NULL;
    size_t size   = MWIFI_PAYLOAD_LEN;
    //uint8_t dest_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type       = {0x0};

    MDF_LOGI("root_write_task is start");
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);

    
    esp_wifi_ap_get_sta_list(&wifi_sta_list);

    for(;;) {
        //MDF_LOGI("root_write_task loop start");
        if (!mwifi_is_connected()) {   
            vTaskDelay(500 / portTICK_RATE_MS);
            esp_wifi_ap_get_sta_list(&wifi_sta_list);
            //MDF_LOGI("root_write_task loop goto");
            continue;
        }
        
        if(ToorRe_status == Receive_messages ||ToorRe_status == Send_message)
        {
            size = asprintf(&data, "{\"user\": \"root\",\"data\": \"WARRING\"}");//根节点发出的消息
            for (int i = 0; i < wifi_sta_list.num; i++) {
                mwifi_root_write(wifi_sta_list.sta[i].mac, 1, &data_type, data, size, true);
            }
            MDF_FREE(data);
        }else if(ToorRe_status == Unknow_messages)
        {
            size = asprintf(&data, "{\"user\": \"root\",\"data\": \"NOTHIONG\"}");//根节点发出的消息
            for (int i = 0; i < wifi_sta_list.num; i++) {
                mwifi_root_write(wifi_sta_list.sta[i].mac, 1, &data_type, data, size, true);
            }
            MDF_FREE(data);
        }

        memset(&wifi_sta_list,0,sizeof(wifi_sta_list));
        esp_wifi_ap_get_sta_list(&wifi_sta_list);
        MDF_LOGI("root_write_task loop end please wait 5s");
        vTaskDelay(10000 / portTICK_RATE_MS);
    }
    MDF_FREE(data);
    MDF_LOGW("root_write_task is exit");
    vTaskDelete(NULL);
}

static void node_write_task(void *arg)
{
    size_t size                     = 0;
    //int count                       = 0;
    char *data                      = NULL;
    //mdf_err_t ret                   = MDF_OK;
    mwifi_data_type_t data_type     = {0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};

    MDF_LOGI("Note write task is running");

    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);

    for (;;) {
        if (!mwifi_is_connected()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }
        
        if(ToorRe_status == Send_message) //火灾
        {
            size = asprintf(&data, "{\"user\": \"node\",\"data\": \"WARRING\"}");//向根节点发出消息
            MDF_LOGI("Note write WARRING");
        }
        else if(ToorRe_status == Unknow_messages)
        {
            size = asprintf(&data, "{\"user\": \"node\",\"data\": \"NOTHING\"}");//向根节点发出消息
            MDF_LOGI("Note write nothing");
        }
        
        mwifi_write(NULL, &data_type, data, size, true);
        MDF_FREE(data);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    MDF_FREE(data);
    MDF_LOGI("Note write task is end");

    vTaskDelete(NULL);
}

static void print_system_info_timercb(void *timer)
{
    uint8_t primary                 = 0;
    wifi_second_chan_t second       = 0;
    mesh_addr_t parent_bssid        = {0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    mesh_assoc_t mesh_assoc         = {0x0};
    wifi_sta_list_t wifi_sta_list   = {0x0};

    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    esp_wifi_get_channel(&primary, &second);
    esp_wifi_vnd_mesh_get(&mesh_assoc);
    esp_mesh_get_parent_bssid(&parent_bssid);

    MDF_LOGI("self mac: " MACSTR ", parent bssid: " MACSTR", free heap: %u",
             MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),esp_get_free_heap_size());

    for (int i = 0; i < wifi_sta_list.num; i++) {
        MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
    }

}

static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: %d", event);

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            
            break;
        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
            MDF_LOGI("MDF_EVENT_MWIFI_ROUTING_TABLE_ADD total_num: %d", esp_mesh_get_total_node_num());
        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
            MDF_LOGI("MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE total_num: %d", esp_mesh_get_total_node_num());
            break;

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: {
            MDF_LOGI("Root obtains the IP address. It is posted by LwIP stack automatically");
            xTaskCreate(root_read_task, "root_read_task", 4 * 1024,
               NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
            xTaskCreate(root_write_task, "root_write_task", 4 * 1024,
                NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);    
            TimerHandle_t timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS,
               true, NULL, print_system_info_timercb);
            xTimerStart(timer, 0);
            //you can create root task in here
            break;
        }

        default:
            break;
    }

    return MDF_OK;
}

void Mesh_Init(char *wifi_ssid, char *wifi_password)
{

    mwifi_init_config_t cfg = MWIFI_INIT_CONFIG_DEFAULT();
    mwifi_config_t config     = {
        .mesh_id         = "123456",
        .mesh_password   = "12345678",
    };

    strcpy((char *)config.router_ssid, "PLAYING");
    strcpy((char *)config.router_password, "alskdjfhg");

    if(mwifi_is_started()){
        MDF_LOGI("mwifi is restarted !!!!!!!!!!!!!");
        mwifi_init(&cfg);
        mwifi_set_config(&config);
        mwifi_restart();
    }
    else{
        MDF_LOGI("mwifi is start !!!!!!!!!!!!!");
        mdf_event_loop_init(event_loop_cb);
        //MDF_ERROR_ASSERT(wifi_init());

        MDF_ERROR_ASSERT(mwifi_init(&cfg));
        MDF_ERROR_ASSERT(mwifi_set_config(&config));
        MDF_ERROR_ASSERT(mwifi_start());

        const uint8_t group_id_list[2][6] = {{0x01, 0x00, 0x5e, 0xae, 0xae, 0xae},
                                            {0x01, 0x00, 0x5e, 0xae, 0xae, 0xaf}};

        MDF_ERROR_ASSERT(esp_mesh_set_group_id((mesh_addr_t *)group_id_list, 
                                    sizeof(group_id_list)/sizeof(group_id_list[0])));

        xTaskCreate(node_read_task, "node_read_task", 4 * 1024,
                    NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
        xTaskCreate(node_write_task, "node_write_task", 4 * 1024,
                    NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    }

}
/*
mdf_err_t Send_Mesh(uint32_t fire_level,char *data,size_t size,mwifi_data_type_t data_type){

    if(!mwifi_is_connected())
    {
        MDF_LOGI("mwifi is not connected can't send message");
        return MDF_FAIL;
    }
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    if(fire_level == FIRE){
        mesh_status = FIRE;
        if(esp_mesh_is_root())
        {   
            size = asprintf(&data, "{\"user\": \"node\",\"data\": \"WARRING\"}");//向根节点发出消息
            mwifi_write(NULL, &data_type, data, size, true);
            MDF_LOGI("already send to root warring");
        }
        else{
            size = asprintf(&data, "{\"user\": \"root\",\"data\": \"WARRING\"}");//根节点发出的消息
            for (int i = 0; i < wifi_sta_list.num; i++) {
                mwifi_root_write(wifi_sta_list.sta[i].mac, 1, &data_type, data, size, true);
            }
            MDF_LOGI("already send to node warring");
        }
    }
    else if(fire_level == NOTH){
        mesh_status = NOTH;
        if(esp_mesh_is_root())
        {
            size = asprintf(&data, "{\"user\": \"node\",\"data\": \"NOTHING\"}");//向根节点发出消息
            mwifi_write(NULL, &data_type, data, size, true);
            MDF_LOGI("already send to root nothing");
        }
        else{
            size = asprintf(&data, "{\"user\": \"root\",\"data\": \"NOTHING\"}");//根节点发出的消息
            for (int i = 0; i < wifi_sta_list.num; i++) {
                mwifi_root_write(wifi_sta_list.sta[i].mac, 1, &data_type, data, size, true);
            }
            MDF_LOGI("already send to node nothing");
        }        
    }
    
    MDF_FREE(data);
    MDF_LOGI("already send");
    return MDF_OK;
}*/