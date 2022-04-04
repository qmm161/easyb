#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "easy_board.h"
#include "msg_queue.h"
#include <mdm/data_repo.h>
#include "app.h"
//-------------------------------------------------------------------------------------
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
//-------------------------------------------------------------------------------------
extern int Mplayer_fd; 
extern int initAudioDevice();

//============================================================================
//执行Mplayer启动播放play命令
//============================================================================
static int play_audio(cJSON *input)
{

	cJSON *url = input->child;
	if(strcmp("url", url->string))
	{
		LOG_WARN("invalid play audio msg.");
		return -1;
	}

	struct mdd_node *node = NULL;
	int rt = repo_get("Data/Audio/Volumn", &node);
	if(rt)
	{
		LOG_WARN("Failed to get volumn para");
		return rt;
	}

        initAudioDevice();

	char cmd[250];
	snprintf(cmd, 250, "loadfile %s\n", url->valuestring);
	 
	write(Mplayer_fd, cmd, strlen(cmd));
	 return 0;
}
//============================================================================
//执行Mplayer停止播放stop命令
//============================================================================
static int stop_play_audio(cJSON *input)
{
    (void) input;
    const char *cmd = "stop\n";
    write( Mplayer_fd, cmd, strlen(cmd) );
    return 0;
}
//============================================================================
//执行Mplayer静音播放mute命令
//============================================================================
static int mute_play_audio(cJSON *input)
{
    (void) input;
    static int mute_value = 0;
    mute_value = mute_value == 0 ? 1 : 0;
    LOG_WARN("try to mute %d.", mute_value);

    char cmd[50];
    snprintf(cmd, 50, "mute %d\n", mute_value);
    write( Mplayer_fd, cmd, strlen(cmd));

    return 0;
}

//============================================================================
//Mplayer控制命令解析
//============================================================================
int handler_app_msg(mqtt_msg *msg)
{
    int rt = 0;
	 //播放命令
    if(!strcmp(msg->msg_name, "PlayAudio"))
	  {
        rt = play_audio(msg->body);
      }
     //停止命令	  
	else if(!strcmp(msg->msg_name, "StopAudio"))
	  {
        rt = stop_play_audio(msg->body);
      }
	 //静音命令 
	else if(!strcmp(msg->msg_name, "MuteAudio")){
        rt = mute_play_audio(msg->body);
    }
	
    return rt;
}
