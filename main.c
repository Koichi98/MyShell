#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include "parse.h"

volatile int waiting_pid;

void print_job_list(job*);

void sig_ign(struct sigaction* act){
    act->sa_sigaction = NULL;
    act->sa_handler = SIG_IGN;
    sigset_t sigset;
    int sigem;
    if((sigem=sigemptyset(&sigset))<0){perror("sigemptyset error");exit(1);}
    act->sa_mask= sigset;
    act->sa_flags=0;
    act->sa_restorer=NULL;
}

void sig_ign_drop(struct sigaction* act){
    act->sa_sigaction = NULL;
    act->sa_handler = NULL;
    sigset_t sigset;
    int sigem;
    if((sigem=sigemptyset(&sigset))<0){perror("sigemptyset error");exit(1);}
    act->sa_mask=sigset;
    act->sa_flags=0;
    act->sa_restorer=NULL;
}

void wait_child(int signal,siginfo_t* info,void* ctx){
    int wstatus;
    if ((waiting_pid = waitpid(info->si_pid,&wstatus, WUNTRACED))<0){perror("waitchild error");}
    printf("accepted");//For debug
}

void sig_wait_child(struct sigaction*act){
    act->sa_handler = NULL;
    act->sa_sigaction = wait_child;
    sigset_t sigset;
    int sigem;
    if((sigem=sigemptyset(&sigset))<0){perror("sigemptyset error");exit(1);}
    act->sa_mask=sigset;
    act->sa_flags = SA_SIGINFO;
    act->sa_restorer = NULL;  
}


int main(int argc, char *argv[],char *envp[]) {
    char s[LINELEN];
    job *curr_job;
    

    struct sigaction bgact;
    sig_wait_child(&bgact);
    struct sigaction act;
    sig_ign(&act);
    struct sigaction newact;
    sig_ign_drop(&newact);
    int sig;

    while(get_line(s, LINELEN)) {
        if((sig = sigaction(SIGINT,&act,NULL))<0){perror("sigaction error");exit(1);}
        if((sig = sigaction(SIGTTOU,&act,NULL))<0){perror("sigaction error");exit(1);}
        if((sig = sigaction(SIGCHLD,&act,NULL))<0){perror("sigaction error");exit(1);}
        if((sig = sigaction(SIGCHLD,&bgact,NULL))<0){perror("sigaction error");exit(1);}
        //Ignore signals:SIGINT,SIGTTOU,SIGCHLD
        if(!strcmp(s, "exit\n")){
            break;
        }
        if(strcmp(s,"\n")){

        
        curr_job = parse_line(s);

        process* cur_process = curr_job->process_list;
        char* newargv[16];
        for(int i=0;i<16;i++){
            newargv[i] = cur_process->argument_list[i];
        }

        if(cur_process->next!=NULL){//pipe
            if(cur_process->next->output_redirection!=NULL){//pipe - output redirection
                if(cur_process->input_redirection!=NULL){//input redirection - pipe - output redirection
                    pid_t pid1,pid2;
                    int fd[2];
                    int p;
                    if((p=pipe(fd))<0){perror("pipe error");}
                    if((pid1=fork())==0){ //パイプの受信側
                        if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}

                        int ofd;
                        if((ofd=open(cur_process->next->output_redirection,O_WRONLY|O_CREAT,S_IRWXU))<0){perror("open error");}
                        dup2(ofd,1);
                        char* newargv[16];
                        for(int i=0;i<16;i++){
                            newargv[i] = cur_process->next->argument_list[i];
                        }
                        dup2(fd[0],0);
                        close(fd[1]);
                        int ret = execve(cur_process->next->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    if((pid2=fork())==0){//パイプの送信側
                        if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}

                        int ifd;
                        if((ifd=open(cur_process->input_redirection,O_RDONLY))<0){perror("open error");}
                        dup2(ifd,0);
                        dup2(fd[1],1);
                        close(fd[0]);
                        int ret = execve(cur_process->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    close(fd[0]);
                    close(fd[1]);
                    setpgid(pid1,0);
                    setpgid(pid2,pid1);
                    if(curr_job->mode!=BACKGROUND){
                        tcsetpgrp(2,pid1);
                        while(waiting_pid != (pid1 || pid2)){
                        }
                        while(waiting_pid != (pid1 || pid2)){
                        }
                    };
                    tcsetpgrp(2,getpid());
                }else{//no input redirection - pipe - output redirection 
                    pid_t pid1,pid2;
                    int fd[2];
                    int p;
                    if((p=pipe(fd))<0){perror("pipe error");}
                    if((pid1=fork())==0){ //パイプの受信側
                        if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}

                        int ofd;
                        if((ofd=open(cur_process->next->output_redirection,O_WRONLY|O_CREAT,S_IRWXU))<0){perror("open error");}
                        dup2(ofd,1);
                        char* newargv[16];
                        for(int i=0;i<16;i++){
                            newargv[i] = cur_process->next->argument_list[i];
                        }
                        dup2(fd[0],0);
                        close(fd[1]);
                        int ret = execve(cur_process->next->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    if((pid2=fork())==0){//パイプの送信側
                        if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}

                        dup2(fd[1],1);
                        close(fd[0]);
                        int ret = execve(cur_process->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    close(fd[0]);
                    close(fd[1]);
                    setpgid(pid1,0);
                    setpgid(pid2,pid1);
                    if(curr_job->mode!=BACKGROUND){
                        tcsetpgrp(2,pid1);
                        while(waiting_pid != (pid1 || pid2)){
                        }
                        while(waiting_pid != (pid1 || pid2)){
                        }
                    }
                    tcsetpgrp(2,getpid());
                }
            }else{//pipe-no output redirection 
                if(cur_process->input_redirection!=NULL){//input - pipe - no output redirection
                    pid_t pid1,pid2;
                    int fd[2];
                    int p;
                    if((p=pipe(fd))<0){perror("pipe error");}
                    if((pid1=fork())==0){ //パイプの受信側
                        if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}

                        char* newargv[16];
                        for(int i=0;i<16;i++){
                            newargv[i] = cur_process->next->argument_list[i];
                        }
                        dup2(fd[0],0);
                        close(fd[1]);

                        int ret = execve(cur_process->next->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    if((pid2=fork())==0){//パイプの送信側
                        if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}

                        int ifd;
                        if((ifd=open(cur_process->input_redirection,O_RDONLY))<0){perror("open error");}
                        dup2(ifd,0);
                        dup2(fd[1],1);
                        close(fd[0]);;;
                        int ret = execve(cur_process->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    close(fd[0]);
                    close(fd[1]);
                    setpgid(pid1,0);
                    setpgid(pid2,pid1);
                    if(curr_job->mode!=BACKGROUND){
                        tcsetpgrp(2,pid1);
                        while(waiting_pid != (pid1 || pid2)){
                        }
                        while(waiting_pid != (pid1 || pid2)){
                        }
                    }
                    tcsetpgrp(2,getpid());
                }else{//no input - pipe - no output redirection
                    pid_t pid1,pid2;
                    int fd[2];
                    int p;
                    if((p=pipe(fd))<0){perror("pipe error");}
                    if((pid1=fork())==0){ //パイプの受信側
                        //setpgid(getpid(),0);
                        //tcsetpgrp(2,getpid());
                        if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}

                        char* newargv[16];
                        for(int i=0;i<16;i++){
                            newargv[i] = cur_process->next->argument_list[i];
                        }
                        dup2(fd[0],0);
                        close(fd[1]);
                        int ret = execve(cur_process->next->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    if((pid2=fork())==0){//パイプの送信側
                        //setpgid(getpid(),pid1);
                        if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}

                        dup2(fd[1],1);
                        close(fd[0]);
                        int ret = execve(cur_process->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    close(fd[0]);
                    close(fd[1]);
                    setpgid(pid1,0);
                    setpgid(pid2,pid1);
                    if(curr_job->mode!=BACKGROUND){
                        tcsetpgrp(2,pid1);
                        while(waiting_pid != (pid1 || pid2)){
                        }
                        while(waiting_pid != (pid1 || pid2)){
                        }
                    }
                    tcsetpgrp(2,getpid());
                }
            }
        }else{//no pipe
            if(cur_process->output_redirection!=NULL){//no pipe - output redirection
                if(cur_process->input_redirection!=NULL){//input- no pipe - output redirection
                    pid_t pid;
                    if((pid=fork())==0){//inputファイル受信側,outputファイル送信側
                        if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}

                        int ifd;
                        if((ifd=open(cur_process->input_redirection,O_RDONLY))<0){perror("open error");}
                        dup2(ifd,0);
                        int ofd;
                        if((ofd=open(cur_process->output_redirection,O_WRONLY|O_CREAT,S_IRWXU))<0){perror("open error");}
                        dup2(ofd,1);
                        int ret = execve(cur_process->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    setpgid(pid,0);
                    if(curr_job->mode!=BACKGROUND){
                        tcsetpgrp(2,pid);

                        while(waiting_pid != pid){
                        }
                    }
                    tcsetpgrp(2,getpid());
                }else{//no input - no pipe - output redirection
                    pid_t pid;
                    if((pid=fork())==0){
                        if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}

                        int ofd;
                        if((ofd=open(cur_process->output_redirection,O_WRONLY|O_CREAT,S_IRWXU))<0){perror("open error");}
                        dup2(ofd,1);
                        int ret = execve(cur_process->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    setpgid(pid,0);
                    if(curr_job->mode!=BACKGROUND){
                        tcsetpgrp(2,pid);
                        while(waiting_pid != pid){
                        }
                    }
                    tcsetpgrp(2,getpid());
                }
            }else{//no pipe-no output redirection
                if(cur_process->input_redirection!=NULL){//input- no pipe - no output redirection
                    pid_t pid;
                    if((pid=fork())==0){//inputファイル受信側
                        if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}

                        int ifd;
                        if((ifd=open(cur_process->input_redirection,O_RDONLY))<0){perror("open error");}
                        dup2(ifd,0);
                        int ret = execve(cur_process->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    setpgid(pid,0);
                    if(curr_job->mode!=BACKGROUND){
                        tcsetpgrp(2,pid);
                        while(waiting_pid != pid){
                        }
                    }
                    tcsetpgrp(2,getpid());
                }else{//no input - no pipe - no output redirection
                    pid_t pid;
                    if((pid=fork())==0){
                        if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}
                        
                        int ret = execve(cur_process->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }else{
                        setpgid(pid,0);
                        if(curr_job->mode!=BACKGROUND){
                            tcsetpgrp(2,pid);
                            while(pid != waiting_pid){
                            }
                        }
                        tcsetpgrp(2,getpid());
                    }
                }
            }
        }

       // print_job_list(curr_job);

        free_job(curr_job);
        }
    }

    return 0;
}
