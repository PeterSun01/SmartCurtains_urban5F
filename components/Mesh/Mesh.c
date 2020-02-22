#include "Mesh.h"


static const char *TAG = "Mesh";

static mdf_err_t event_loop_cb(mdf_event_loop_t event, void *ctx)
{
    MDF_LOGI("event_loop_cb, event: %d", event);

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
            MDF_LOGI("Root obtains the IP address. It is posted by LwIP stack automatically");
            break;

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

        const uint8_t group_id_list[2][6] = {{0x01, 0x00, 0x5e, 0xae, 0xae, 0xae},
                                            {0x01, 0x00, 0x5e, 0xae, 0xae, 0xaf}};

        esp_mesh_set_group_id((mesh_addr_t *)group_id_list, 
        sizeof(group_id_list)/sizeof(group_id_list[0]));

        //xTaskCreate(node_read_task, "node_read_task", 4 * 1024,
        //            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
        //xTaskCreate(node_write_task, "node_write_task", 4 * 1024,
        //            NULL, CONFIG_MDF_TASK_DEFAULT_PRIOTY, NULL);
    }

}