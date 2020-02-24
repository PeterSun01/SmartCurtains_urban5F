#include "Mesh.h"


static const char *TAG = "Mesh";
TaskHandle_t root_read_xhandle,root_write_xhandle;
TaskHandle_t node_read_xhandle,node_write_xhandle;
TimerHandle_t timer;

void root_read_task(void *arg)
{
    
    char *data    = NULL;
    size_t size   = MWIFI_PAYLOAD_LEN;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0x0};
    
    mdf_err_t ret = MDF_OK;
    MDF_LOGI("root_read_task running");



    for(;;) {
        if(mwifi_is_connected() && esp_mesh_get_layer() != MESH_ROOT){ //判断当前节点是否为根节点在进行工作
            vTaskDelay(500 / portTICK_RATE_MS);
            MDF_LOGI("root_read_task loop");
            continue;
        }
    
        //mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        //MDF_LOGI("ROOT receive1: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);
        ret = mwifi_root_read(src_addr, &data_type, &data, &size, portMAX_DELAY);
        MDF_ERROR_GOTO(ret != MDF_OK, MEM_FREE, "<%s> mwifi_root_read", mdf_err_to_name(ret));
        
        parse_rootread_mesh(data);
        //MDF_LOGI("ROOT receive1: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);
MEM_FREE:
        MDF_FREE(data);
    }
    MDF_FREE(data);
    vTaskDelete(NULL);
}

void root_write_task(void *arg)
{

    char *data    = NULL;
    size_t size   = MWIFI_PAYLOAD_LEN;
    //uint8_t dest_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0x0};
    wifi_sta_list_t wifi_sta_list   = {0x0};

    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    MDF_LOGI("Root write task is running");

    for(;;) {
        if(mwifi_is_connected() && esp_mesh_get_layer() != MESH_ROOT){ //判断当前节点是否为根节点在进行工作
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        vTaskDelay(3000 / portTICK_RATE_MS);
        MDF_FREE(data);
    }
    MDF_FREE(data);
    vTaskDelete(NULL);
}


static void node_read_task(void *arg)
{
    char *data    = MDF_MALLOC(MWIFI_PAYLOAD_LEN);
    size_t size   = MWIFI_PAYLOAD_LEN;
    mwifi_data_type_t data_type      = {0x0};
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mdf_err_t ret = MDF_OK;
    MDF_LOGI("Node read task is running");

    for (;;) {
        if (!mwifi_is_connected()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }
        
        mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        ret = mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        MDF_ERROR_CONTINUE(ret != MDF_OK, "<%s> mwifi_read", mdf_err_to_name(ret));

        //MDF_LOGI("Node receive: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);
        memset(data, '\0', size);
        
    }
    MDF_FREE(data);
    MDF_LOGW("Node read task is exit");
    vTaskDelete(NULL);
}


static void node_write_task(void *arg)
{
    size_t size   = 0;
    char *data    = NULL;
    mwifi_data_type_t data_type     = {0x0};

    MDF_LOGI("Node task is running");
    int i=0;
    for (;;) {
        
        if (!mwifi_is_connected() ) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

        
        MDF_FREE(data);
        vTaskDelay(3000 / portTICK_RATE_MS);
    }
    

    vTaskDelete(NULL);
}
/*
static void print_system_info_timercb(void *timer) //定时器查看当前剩余内存 
{
    wifi_sta_list_t wifi_sta_list   = {0x0};

    esp_wifi_ap_get_sta_list(&wifi_sta_list);

    MDF_LOGI("node num %d",wifi_sta_list.num);


}
*/
void Send_Test(){
        size_t size   = 5;
        mwifi_data_type_t data_type     = {0x0};
        mwifi_write(NULL, &data_type, "Mined", size, true);
}

static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: %d", event); //显示callback问题编号
    

    switch (event) {
        case MDF_EVENT_MWIFI_STARTED:
            MDF_LOGI("MESH is started");
            
            break;
        case MDF_EVENT_MWIFI_PARENT_CONNECTED:
            MDF_LOGI("Parent is connected on station interface");
            break;

        case MDF_EVENT_MWIFI_PARENT_DISCONNECTED:
            MDF_LOGI("Parent is disconnected on station interface");
            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_ADD:
            MDF_LOGI("MDF_EVENT_MWIFI_ROUTING_TABLE_ADD");
            break;

        case MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE:
            MDF_LOGI("MDF_EVENT_MWIFI_ROUTING_TABLE_REMOVE");
            break;

        case MDF_EVENT_MWIFI_ROOT_ADDRESS:
            MDF_LOGI("MDF_EVENT_MWIFI_ROOT_ADDRESS");
            if(!esp_mesh_is_root())
               Send_Test(); //此处可以发送发送设备信息
            break;
        case MDF_EVENT_MWIFI_ROOT_GOT_IP: 
            MDF_LOGI("MDF_EVENT_MWIFI_ROOT_GOT_IP");
            
            xTaskCreate(root_read_task, "root_read", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &root_read_xhandle);
            xTaskCreate(root_write_task, "write_read", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &root_write_xhandle);

            
            //timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS,true, NULL, print_system_info_timercb);
            //xTimerStart(timer, 0);

            vTaskDelete(node_write_xhandle);
            vTaskDelete(node_read_xhandle);
            break;
        case MDF_EVENT_MWIFI_ROOT_LOST_IP:
            MDF_LOGI("MDF_EVENT_MWIFI_ROOT_LOST_IP");
            //xTimerStop(timer, 0);
            vTaskDelete(root_read_xhandle);
            vTaskDelete(root_write_xhandle);
            xTaskCreate(node_read_task, "node_read_task", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &node_read_xhandle);
            xTaskCreate(node_write_task, "node_write_task", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &node_write_xhandle);
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
    const uint8_t group_id_list[2][6] = {{0x01, 0x00, 0x5e, 0xae, 0xae, 0xae},
                                        {0x01, 0x00, 0x5e, 0xae, 0xae, 0xaf}};
                                        
    strcpy((char *)config.router_ssid, wifi_ssid);
    strcpy((char *)config.router_password, wifi_password);


    if(mwifi_is_started()){
        MDF_LOGI("mwifi restarted !!!!!!!!!!!!!");
        mwifi_init(&cfg);
        mwifi_set_config(&config);
        mwifi_restart();

    }else{
        MDF_LOGI("mwifi start !!!!!!!!!!!!!");
        mdf_event_loop_init(event_loop_cb);

        mwifi_init(&cfg);
        mwifi_set_config(&config);
        mwifi_start();
        xTaskCreate(node_read_task, "node_read_task", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &node_read_xhandle);
        xTaskCreate(node_write_task, "node_write_task", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &node_write_xhandle);

        esp_mesh_set_group_id((mesh_addr_t *)group_id_list, sizeof(group_id_list)/sizeof(group_id_list[0]));
    }

}



mdf_err_t Send_Fire_Mesh(uint32_t fire_level){

    size_t size                     = 0;
    //int count                       = 0;
    char *data                      = NULL;
    //mdf_err_t ret                   = MDF_OK;
    mwifi_data_type_t data_type     = {0};
    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    wifi_sta_list_t wifi_sta_list   = {0x0};

    if(!mwifi_is_connected())
    {
        MDF_LOGI("mwifi is not connected can't send message");
        return MDF_FAIL;
    }
    MDF_LOGI("Ready...");
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    if(fire_level == FIRE){
        mesh_status = FIRE;
        if(!esp_mesh_is_root())
        {   
            size = asprintf(&data, "{\"User\": \"NODE\",\"Data\": \"WARNING\",\"Type\": \"FIRESTA\"}");//向根节点发出消息
            mwifi_write(NULL, &data_type, data, size, true);
            MDF_LOGI("already send to root warring");
        }
        else{
            size = asprintf(&data, "{\"User\": \"ROOT\",\"Data\": \"WARNING\",\"Type\": \"FIRESTA\"}");//根节点发出的消息
            for (int i = 0; i < wifi_sta_list.num; i++) {
                mwifi_root_write(wifi_sta_list.sta[i].mac, 1, &data_type, data, size, true);
            }
            MDF_LOGI("already send to node warring");
        }
    }
    else if(fire_level == NOTH){
        mesh_status = NOTH;
        if(!esp_mesh_is_root())
        {
            size = asprintf(&data, "{\"User\": \"NODE\",\"Data\": \"NOTHING\",\"Type\": \"FIRESTA\"}");//向根节点发出消息
            mwifi_write(NULL, &data_type, data, size, true);
            MDF_LOGI("already send to root nothing");
        }
        else{
            size = asprintf(&data, "{\"User\": \"ROOT\",\"Data\": \"NOTHING\",\"Type\": \"FIRESTA\"}");//根节点发出的消息
            for (int i = 0; i < wifi_sta_list.num; i++) {
                mwifi_root_write(wifi_sta_list.sta[i].mac, 1, &data_type, data, size, true);
            }
            MDF_LOGI("already send to node nothing");
        }        
    }
    
    MDF_FREE(data);
    return MDF_OK;
}

mdf_err_t parse_rootread_mesh(char *rootdata){
    cJSON *json_data_parse              = NULL;
    cJSON *json_data_parse_type         = NULL; //解析的信息属于什么状态
    cJSON *json_data_parse_firesta      = NULL;
    cJSON *json_data_parse_productid    = NULL;
    cJSON *json_data_parse_channelid    = NULL;
    cJSON *json_data_parse_apikey       = NULL;
    cJSON *json_data_parse_serialnum    = NULL;

    
    if((json_data_parse = cJSON_Parse(rootdata)) !=NULL)
    {   
        if((json_data_parse_type = cJSON_GetObjectItem(json_data_parse, "Type")) != NULL)
        {   
            if(strcmp(json_data_parse_type->valuestring,MEST.FIRESTA) == PARSE_OK)
            {   
                if((json_data_parse_firesta = cJSON_GetObjectItem(json_data_parse, "Data"))!= NULL)
                {   
                    if(strcmp(json_data_parse_firesta->valuestring,MEST.WARNING) == PARSE_OK ) //且当前状态不为火灾
                    {   
                        Device_status = Receive_Device;  
                        MDF_LOGI("Json Warning ......");
                        //gpio_set_level(27,0);
                        

                    }else if(strcmp(json_data_parse_firesta->valuestring,MEST.NOTHING) == PARSE_OK) //且当前状态不为无事
                    {  
                        Device_status = Default_Device;
                        MDF_LOGI("Json Nothing ......");
                        gpio_set_level(27,1);
                    }
                }
            }else if(strcmp(json_data_parse_type->valuestring,MEST.PROJSTA) == PARSE_OK)
            {
                json_data_parse_productid = cJSON_GetObjectItem(json_data_parse, "ProductId");
                json_data_parse_channelid = cJSON_GetObjectItem(json_data_parse, "SerialNum");
                json_data_parse_apikey    = cJSON_GetObjectItem(json_data_parse, "ApiKey");
                json_data_parse_serialnum = cJSON_GetObjectItem(json_data_parse, "ChannelId");
            }
        }

    }

    cJSON_Delete(json_data_parse);
    return MDF_OK;
}