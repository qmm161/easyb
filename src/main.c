//=================================================================================================
//主程序控制Mplayer的播放
//方法二：从有名管道(fifo)输入控制命令（应用编程中使用）
// #mkfifo </tmp/fifofile>
// #mplayer  -slave  -input  file=</tmp/fifofile> <movie>
//用户可以通过往管道里写入slave命令来实现对应的功能
//例：主进程创建一个无名管道和一个有名管道
//1:开一个子进程
//在子进程中：
//启动Mplayer，参数规定通过命名管道进行通信；
//把子进程的标准输出重定向无名管道的写端；
//Mplayer从命名管道读到主进程发送的命令;
//Mplayer发出的内容发送到无名管道中，父进程通过读管道就可以读到Mplayer发出的信息。
//2：在父进程中：
//启动两个线程
//第一个线程，不断使用fgets从键盘获取一个字符串命令，并写入命名管道中
//第二个线程，循环检测无名管道是否有信息可读，有信息将其打印输出在屏幕上
//==============================================================================================
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <MQTTClient.h>
#include <mdm/data_repo.h>

#include "easy_board.h"
#include "msg_queue.h"d
#include "app.h"

//------------------------------------------------------------
//**********************全局变量定义区*****************
int fd_fifo;					//创建有名管道，用于向mplayer发送命令
int fd_pipe[2];					//创建无名管道,用于从mplayer读取命令

//#define FIFO    "fifo"   
//int  fd;  
//int  pipedes[2];  
//char  msg_buf[REC_MSG_CHNUM ] ; /*接收到信息缓冲区*/   
//int  my_lock=0; //发送命令标志位   
//-----------------------------------------------------------
#define CLIENTID    "123456|securemode=3,signmethod=hmacsha1|"
#define QOS         1
#define TIMEOUT     10000L

volatile MQTTClient_deliveryToken deliveredtoken;


static int set_data(cJSON *input)
{
    int rt = repo_edit_json(input);
    LOG_WARN("try edit data with rlt:%d", rt);
    return 0;
}

static int get_data(cJSON *input)
{
    (void) input;
    return 0;
}

static int handler_mqtt_msg(mqtt_msg *msg)
{
    int rt = 0;
    if(!strcmp(msg->msg_name, "Set")){
        rt = set_data(msg->body);
    } else if(!strcmp(msg->msg_name, "Get")){
        rt = get_data(msg->body);
    } else {
        rt = handler_app_msg(msg);
    }

    return rt;
}

static void msg_delivered(void *context, MQTTClient_deliveryToken dt)
{
    (void) context;
    LOG_INFO("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

static int msg_arrived(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    (void) context;
    (void) topicLen;

    char *payload = (char*) message->payload;
    int len = message->payloadlen;
    CHECK_DO_GOTO(payload[len] != '\0', LOG_WARN("Received msg is not valid string"), EXIT);

    LOG_INFO("Message arrived");
    LOG_INFO("   topic: %s", topicName);
    LOG_INFO("   message: %s", payload);

    mqtt_msg *mq_msg = malloc_mqtt_msg(topicName, payload);
    CHECK_DO_GOTO(!mq_msg, LOG_WARN("Failed to build mqtt_msg!"), EXIT);

    int rt = msg_enqueue(mq_msg);
    if(rt){
        LOG_WARN("Failed to enqueue mqtt_msg!");
        free_mqtt_msg(mq_msg);
    }

EXIT:
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

static void conn_lost(void *context, char *cause)
{
    (void) context;
    LOG_INFO("\nConnection lost\n");
    LOG_INFO("     cause: %s\n", cause);
}

static int mqtt_subscribe_type_topic(MQTTClient client, char *buf, size_t buf_len, const char *type_name,
        const char *types, const char *def_type)
{
    if(types){
        char *buf_dup = strdup(types);
        CHECK_DO_RTN_VAL(!buf_dup, LOG_WARN("No memory"), -1);

        LOG_INFO("try sub topics: %s", buf_dup);

        char *begin = buf_dup;
        char *cur = begin;
        while((*cur != '\0') && (*begin != '\0')){
            if(*cur == '|'){
                *cur = '\0';
                snprintf(buf, buf_len, "vc100/bcast/%s/%s", type_name, begin);
                LOG_INFO("subscribe topic: %s", buf);
                MQTTClient_subscribe(client, buf, QOS);

                begin = cur + 1;
            }
            cur++;
        }

        if(begin){
            snprintf(buf, buf_len, "vc100/bcast/%s/%s", type_name, begin);
            LOG_INFO("subscribe topic: %s", buf);
            MQTTClient_subscribe(client, buf, QOS);
        }

        free(buf_dup);
    }

    snprintf(buf, buf_len, "vc100/bcast/%s/%s", type_name, def_type);
    LOG_INFO("subscribe topic: %s", buf);
    MQTTClient_subscribe(client, buf, QOS);

    return 0;
}

static int subscribe_topics(MQTTClient client)
{
    char buf[512];
    struct mdd_node *node = NULL;

    int rt = repo_get("Data/DevId", &node);
    CHECK_DO_RTN_VAL(rt || !node, LOG_WARN("Failed to get devid"), -1);
    char *devid = str_leaf_val(node);
    snprintf(buf, 512, "vc100/cmd/%s", devid);
    LOG_INFO("subscribe topic: %s", buf);
    MQTTClient_subscribe(client, buf, QOS);

    repo_get("Data/Types", &node);
    rt = mqtt_subscribe_type_topic(client, buf, 512, "type", node ? str_leaf_val(node) : NULL, "all");
    CHECK_DO_RTN_VAL(rt, LOG_WARN("Failed to subscribe type topic"), -1);

    repo_get("Data/Areas", &node);
    rt = mqtt_subscribe_type_topic(client, buf, 512, "area", node ? str_leaf_val(node) : NULL, "all");
    CHECK_DO_RTN_VAL(rt, LOG_WARN("Failed to subscribe area topic"), -1);

    return 0;
}

static int mqtt_client_init(MQTTClient *client)
{
    MQTTClient_connectOptions connOpt = MQTTClient_connectOptions_initializer;
    int rt = 0;

    struct mdd_node *node = NULL;
    rt = repo_get("Data/Server/Ip", &node);
    CHECK_DO_RTN_VAL(rt || !node, LOG_WARN("Failed to get Ip"), -1);
    char *ip = str_leaf_val(node);

    rt = repo_get("Data/Server/Port", &node);
    CHECK_DO_RTN_VAL(rt || !node, LOG_WARN("Failed to get Port"), -1);
    int port = int_leaf_val(node);

    char server[100];
    snprintf(server, 100, "tcp://%s:%d", ip, port);
    LOG_INFO("Server url: %s", server);

    rt = MQTTClient_create(client, server, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    CHECK_DO_RTN_VAL(rt != MQTTCLIENT_SUCCESS, LOG_WARN("Failed to create mqtt client:%d", rt), -1);

    connOpt.keepAliveInterval = 20;
    connOpt.cleansession = 1;

    MQTTClient_setCallbacks(*client, NULL, conn_lost, msg_arrived, msg_delivered);

    rt = MQTTClient_connect(*client, &connOpt);
    CHECK_DO_RTN_VAL(rt != MQTTCLIENT_SUCCESS, LOG_WARN("Failed to connect to mqtt borker:%d", rt), -1);

    subscribe_topics(*client);

    return 0;
}

static int mdm_repo_init(char *ws)
{
    char model_path[100];
    char data_path[100];

    strncpy(model_path, ws, 99);
    strncat(model_path, "/model.json", 100 - strlen(model_path) - 1);
    strncpy(data_path, ws, 99);
    strncat(data_path, "/data.json", 100 - strlen(data_path) - 1);
    return repo_init(model_path, data_path);
}

int main(int argc, char *argv[])
{
    if(argc < 2){
        LOG_WARN("Lack para: repo dir!");
        return 0;
    }

    int rt = msg_init_queue();
    LOG_INFO("init msg queue with rlt:%d", rt);

    rt = mdm_repo_init(argv[1]);
    LOG_INFO("init repo with rlt:%d", rt);

    MQTTClient client = NULL;
    rt = mqtt_client_init(&client);
	
	//--------------------------------------------------
    //建立Mplayer的播放管道，用于主程序控制Mplayer播放器
    //建立管道
	//已存在管道，先删除
	unlink( "/tmp/Mplayer_fifo" );
	//创建有名管道，控制Mplayer
    mkfifo( "/tmp/Mplayer_fifo", 0777 );
	
	//--------------------------------------------------
    while(1){
        mqtt_msg *msg = msg_dequeue();
        if(msg){
            LOG_INFO("dequeue msg with topic:%s", msg->topic);
            rt = handler_mqtt_msg(msg);
            LOG_INFO("handler mqtt msg rlt:%d", rt);
            free_mqtt_msg(msg);
        }
    }

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rt;
}
