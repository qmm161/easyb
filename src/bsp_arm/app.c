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
int MplayerRunning=0;
int MplayerCurentFile=0;
//============================================================================
//执行Mplayer停止播放stop命令
//============================================================================
static int stop_play_audio(cJSON *input)
{
    system("ps\n");
	if(MplayerRunning==1)
	 {	
	  (void) input;
      const char *cmd = "stop\n";
      write( Mplayer_fd, cmd, strlen(cmd) );
	  MplayerRunning=0;
	 }
	  system("ps\n");
    return 0;
}
//============================================================================
//执行Mplayer启动播放play命令
//网络播放，需要先做avcode的网络初始化配置，遗留问题
//============================================================================
static int play_audio(cJSON *input)
{
     //启动新的播放文件，先停止
	 // system("ps\n");
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
	 // system("ps\n");
	 return 0;
}

//============================================================================
//执行Mplayer暂停播放pause命令
//============================================================================
static int pause_play_audio(cJSON *input)
{
    (void) input;
    static int pause_value = 0;
    pause_value = pause_value == 0 ? 1 : 0;
    LOG_WARN("try to pause\n ");

    char cmd[50];
    snprintf(cmd, 50, "pause\n");
    write( Mplayer_fd, cmd, strlen(cmd));

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
//问题：需要实现loadflie不同文件的比对，确认播放文件不同才做切换
//============================================================================
int handler_app_msg(mqtt_msg *msg)
{
    int rt = 0;
	 //播放命令
    if(!strcmp(msg->msg_name, "PlayAudio"))
	  {
        if(MplayerRunning==0)
		  {
			 MplayerRunning=1;//先置1，让stop可以执行
			 rt = stop_play_audio(msg->body);	
		     sleep(3);        
	         rt = play_audio(msg->body);
			  MplayerRunning=1;//第2次置1，才是表示正在运行Mplayer
		  }
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
	//暂停命令 
	else if(!strcmp(msg->msg_name, "PauseAudio")){
        rt = pause_play_audio(msg->body);
    }
	
    return rt;
}
