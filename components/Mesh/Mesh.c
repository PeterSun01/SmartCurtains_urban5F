#include "Mesh.h"


static const char *TAG = "Mesh";
TaskHandle_t root_read_xhandle,root_write_xhandle;
TaskHandle_t node_read_xhandle,node_write_xhandle;
void root_read_task(void *arg)
{
    
    char *data    = NULL;
    size_t size   = MWIFI_PAYLOAD_LEN;
    uint8_t src_addr[MWIFI_ADDR_LEN] = {0x0};
    mwifi_data_type_t data_type      = {0x0};
    cJSON *json_data_parse = NULL;

    MDF_LOGI("root_read_task is running");

    for(;;) {
        if(mwifi_is_connected() && esp_mesh_get_layer() != MESH_ROOT){ //判断当前节点是否为根节点在进行工作
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }
        mwifi_root_read(src_addr, &data_type, &data, &size, portMAX_DELAY);
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
        size = asprintf(&data, "{\"user\": \"root\",\"data\": \"HELLO\"}");
 
        for (int i = 0; i < wifi_sta_list.num; i++) {
                mwifi_root_write(wifi_sta_list.sta[i].mac, 1, &data_type, data, size, true);
        }

        memset(&wifi_sta_list,'\0',sizeof(wifi_sta_list));
        esp_wifi_ap_get_sta_list(&wifi_sta_list);
        vTaskDelay(3000 / portTICK_RATE_MS);
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

    MDF_LOGI("Node read task is running");

    for (;;) {
        if (!mwifi_is_connected()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }
        memset(data, '\0', MWIFI_PAYLOAD_LEN);
        mwifi_read(src_addr, &data_type, data, &size, portMAX_DELAY);
        MDF_LOGI("Node receive: " MACSTR ", size: %d, data: %s", MAC2STR(src_addr), size, data);
        
        
    }
    MDF_FREE(data);
    MDF_LOGW("Node read task is exit");
    vTaskDelete(NULL);
}

static void print_system_info_timercb(void *timer) //定时器查看当前剩余内存 
{

    uint8_t sta_mac[MWIFI_ADDR_LEN] = {0};
    wifi_sta_list_t wifi_sta_list   = {0x0};
    mesh_addr_t parent_bssid        = {0};
    
    esp_wifi_get_mac(ESP_IF_WIFI_STA, sta_mac);
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    esp_mesh_get_parent_bssid(&parent_bssid);

    MDF_LOGI("self mac: " MACSTR ", parent bssid: " MACSTR ",free heap: %u", 
             MAC2STR(sta_mac), MAC2STR(parent_bssid.addr),esp_get_free_heap_size());

    for (int i = 0; i < wifi_sta_list.num; i++) {
        MDF_LOGI("Child mac: " MACSTR, MAC2STR(wifi_sta_list.sta[i].mac));
    }

}

static void node_write_task(void *arg)
{
    size_t size   = 0;
    char *data    = NULL;
    mwifi_data_type_t data_type     = {0x0};

    MDF_LOGI("Node task is running");

    for (;;) {
        if (!mwifi_is_connected() || !mwifi_get_root_status()) {
            vTaskDelay(500 / portTICK_RATE_MS);
            continue;
        }

   
        size = asprintf(&data, "{\"user\": \"root\",\"data\": \"HELLO\"}");

        mwifi_write(NULL, &data_type, data, size, true);
        
        MDF_FREE(data);
        vTaskDelay(3000 / portTICK_RATE_MS);
    }
    

    vTaskDelete(NULL);
}



static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    //MDF_LOGI("event_loop_cb, event: %d", event); 显示callback问题编号
    //TimerHandle_t timer;

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

        case MDF_EVENT_MWIFI_ROOT_GOT_IP: 
            MDF_LOGI("MDF_EVENT_MWIFI_ROOT_GOT_IP");
            
            xTaskCreate(root_read_task, "root_read", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &root_read_xhandle);
            xTaskCreate(root_write_task, "write_read", 4 * 1024,NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, &root_write_xhandle);

            //timer = xTimerCreate("print_system_info", 10000 / portTICK_RATE_MS,true, NULL, print_system_info_timercb);
            //xTimerStart(timer, 0);
            
            break;
        case MDF_EVENT_MWIFI_ROOT_LOST_IP:
            MDF_LOGI("MDF_EVENT_MWIFI_ROOT_LOST_IP");
            //xTimerStop(timer, 0);
            vTaskDelete(root_read_xhandle);
            vTaskDelete(root_write_xhandle);
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



mdf_err_t Send_Mesh(uint32_t fire_level){

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
}