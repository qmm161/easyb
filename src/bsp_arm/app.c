#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "easy_board.h"
#include "msg_queue.h"
#include <mdm/data_repo.h>
#include "app.h"

static pthread_t tid;

static void* MplayerRtmp(void *arg)
{
    char *cmd = (char*) arg;
    LOG_WARN("try to play:%s.", cmd);
    system(cmd);
    free(cmd);
    pthread_exit(NULL);
    return NULL;
}

static int play_audio(cJSON *input)
{
    cJSON *url = input->child;
    if(strcmp("url", url->string)){
        LOG_WARN("invalid play audio msg.");
        return -1;
    }

    struct mdd_node *node = NULL;
    int rt = repo_get("Data/Audio/Volumn", &node);
    if(rt){
        LOG_WARN("Failed to get volumn para");
        return rt;
    }

    char *cmd = (char*) calloc(1, 250);
    snprintf(cmd, 250, "/root/play.sh %lld %s", int_leaf_val(node), url->valuestring);
    pthread_create(&tid, NULL, MplayerRtmp, (void*) cmd);
    return 0;
}

static int stop_play_audio(cJSON *input)
{
    (void) input;
    LOG_WARN("try to stop play.");
    pthread_cancel(tid);
    system("killall mplayer");
    return 0;
}

int handler_app_msg(mqtt_msg *msg)
{
    int rt = 0;
    if(!strcmp(msg->msg_name, "PlayAudio")){
        rt = play_audio(msg->body);
    } else if(!strcmp(msg->msg_name, "StopAudio")){
        rt = stop_play_audio(msg->body);
    }
    return rt;
}
