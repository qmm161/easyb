#include <string.h>
#include "msg_queue.h"
#include "rpa_queue.h"
#include <mdm/macro.h>
#include <mdm/log.h>

#define MAX_MSG_CNT 20
#define MSG_QUEUE_TIMEOUT 2000

static rpa_queue_t *msg_queue = NULL;

void free_mqtt_msg(mqtt_msg *msg)
{
    if (msg)
    {
        free(msg->topic);
        free(msg->msg_id);
        free(msg->msg_name);
        cJSON_Delete(msg->body);
        free(msg);
    }
}

static int mqtt_msg_is_valid(mqtt_msg *msg)
{
    return msg && msg->topic && msg->msg_id && msg->msg_name && msg->body ? 1 : 0;
}

mqtt_msg *malloc_mqtt_msg(const char *topic, const char *payload)
{
    cJSON *body = cJSON_Parse(payload);
    CHECK_DO_RTN_VAL(!body, LOG_WARN("Failed to parse msg!"), NULL);

    mqtt_msg *msg = (mqtt_msg *)calloc(1, sizeof(mqtt_msg));
    CHECK_DO_RTN_VAL(!msg, LOG_WARN("No memory!"); cJSON_Delete(body), NULL);

    msg->topic = strdup(topic);
    cJSON *child = body->child;
    while (child)
    {
        if (!strcmp(child->string, "msg_id"))
        {
            msg->msg_id = strdup(child->valuestring);
        }
        else if (!strcmp(child->string, "msg_name"))
        {
            msg->msg_name = strdup(child->valuestring);
        }
        else if (!strcmp(child->string, "msg_body"))
        {
            msg->body = child;
        }
        child = child->next;
    }
    cJSON_DetachItemViaPointer(body, msg->body);
    cJSON_Delete(body);
    CHECK_DO_GOTO(!mqtt_msg_is_valid(msg), LOG_WARN("Failed to parse msg!"), INVALID_MSG);
    return msg;

INVALID_MSG:
    LOG_INFO("Invalid mqtt message");
    free_mqtt_msg(msg);
    return NULL;
}

int msg_init_queue()
{
    rpa_queue_create(&msg_queue, MAX_MSG_CNT);
    CHECK_DO_RTN_VAL(!msg_queue, LOG_WARN("No memory!"), -1);

    return 0;
}

int msg_enqueue(mqtt_msg *msg)
{
    bool rt = rpa_queue_timedpush(msg_queue, (void *)&msg, MSG_QUEUE_TIMEOUT);
    CHECK_DO_RTN_VAL(!rt, LOG_WARN("Failed to enqueue msg!"), -1);

    LOG_INFO("Succ to enqueue msg: %lld", (long long)msg);
    return 0;
}

mqtt_msg *msg_dequeue()
{
    mqtt_msg *msg = NULL;
    bool rt = rpa_queue_timedpop(msg_queue, (void**)&msg, MSG_QUEUE_TIMEOUT);
    CHECK_RTN_VAL(!rt, NULL);

    LOG_INFO("Succ to dequeue msg: %lld", (long long)msg);
    return msg;
}

