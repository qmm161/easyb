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
static pthread_t tid;

static void* MplayerRtmp(void *arg)
{
    char *cmd = (char*) arg;
    LOG_WARN("try to play:%s.", cmd);
	//----------------------------------------------------------------------------------
	//命令行配置
	//shell command 配置播放设备
	FILE *comm_fp_p = NULL;
	char comm_ret[100] = {'0'};
	//选择声卡设备
	comm_fp_p = popen("amixer cset numid=17,iface=MIXER,name='Speaker Function' 0", "r");
 	if(comm_fp_p == NULL)
	    {
 		  printf("shell error\n");
 		  return 0;
 		}
 	while(fgets(comm_ret, sizeof(comm_ret) - 1, comm_fp_p) != NULL)
 	 	  printf("amixer:\n%s\n", comm_ret);
 	pclose(comm_fp_p);
	
	//配置音量
	comm_fp_p = popen("amixer cset numid=1,iface=MIXER,name='Master Playback Volume' 200", "r");
 	if(comm_fp_p == NULL)
	    {
 		  printf("shell error\n");
 		  return 0;
 		}
 	while(fgets(comm_ret, sizeof(comm_ret) - 1, comm_fp_p) != NULL)
 	 	  printf("amixer:\n%s\n", comm_ret);
 	pclose(comm_fp_p);
	
	
    //命令行播放mplayer
	//重新播放，先杀mplayer进程
	system("killall mplayer");
	comm_fp_p = popen(cmd, "r");
 	if(comm_fp_p == NULL)
	    {
 		  printf("shell mplayer error\n");
 		  return 0;
 		}
 	while(fgets(comm_ret, sizeof(comm_ret) - 1, comm_fp_p) != NULL)
 	 	  printf("amixer:\n%s\n", comm_ret);
 	pclose(comm_fp_p);	
	
	//-----------------------------------------------------------------------------------	
   // system(cmd);
   // free(cmd);
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

    //snprintf(cmd, 250, "/root/play.sh %lld %s", int_leaf_val(node), url->valuestring);
	//mplayer配置为工作在slave模式，便于主程序通cmd管道命令来控制mplayer播放器

	snprintf(cmd, 250, "/usr/bin/mplayer -slave -quiet -input file=/tmp/Mplayer_fifo %s & ", url->valuestring);
    pthread_create(&tid, NULL, MplayerRtmp, (void*) cmd);
    return 0;
}
//============================================================================
//有名管道传递命令，控制Mplayer 停止播放
//============================================================================
static int stop_play_audio(cJSON *input)
{
   int fd;
   char s[] = "pause\n";
   if( fork() > 0 )
     {
        fd = open( "/tmp/Mplayer_fifo", O_WRONLY );
        write( fd, s, sizeof(s) );
        close( fd );
    }
    return 0;
}
/*
static int stop_play_audio(cJSON *input)
{
    (void) input;
    LOG_WARN("try to stop play.");
    pthread_cancel(tid);
    system("killall mplayer");
    return 0;
}
*/

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
