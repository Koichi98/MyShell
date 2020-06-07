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

    int sig;
    if((sig = sigaction(SIGINT,act,NULL))<0){perror("sigaction error");exit(1);}
    if((sig = sigaction(SIGTTOU,act,NULL))<0){perror("sigaction error");exit(1);}

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

    int sig;
    if((sig = sigaction(SIGINT,act,NULL))<0){perror("sigaction error");exit(1);}

    
}

void wait_child(int signal,siginfo_t* info,void* ctx){
    int w,wstatus;
    if ((w = waitpid(info->si_pid,&wstatus, WUNTRACED))<0){perror("waitchild error");}
    printf("accepted");
}

int main(int argc, char *argv[],char *envp[]) {
    char s[LINELEN];
    job *curr_job;
    

    struct sigaction bgact;
    bgact.sa_handler = NULL;
    bgact.sa_sigaction = wait_child;
    sigset_t sigset;
    int sigem;
    if((sigem=sigemptyset(&sigset))<0){perror("sigemptyset error");exit(1);}
    bgact.sa_mask=sigset;
    bgact.sa_flags = SA_SIGINFO;
    bgact.sa_restorer = NULL;  


    while(get_line(s, LINELEN)) {
        struct sigaction act;
        sig_ign(&act);//SIGINTの無視を設定
        if(!strcmp(s, "exit\n")){
            break;
        }
        
        curr_job = parse_line(s);

        process* cur_process = curr_job->process_list;
        char* newargv[16];
        for(int i=0;i<16;i++){
            newargv[i] = cur_process->argument_list[i];
        }

        if(cur_process->input_redirection!=NULL){//input
            if(cur_process->output_redirection!=NULL){//input,output
                pid_t pid;
                int wstatus;
                if((pid=fork())==0){//inputファイル受信側,outputファイル送信側
                    struct sigaction newact;
                    sig_ign_drop(&newact);

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
                tcsetpgrp(2,pid);

                int w;
                if ((w = waitpid(pid,&wstatus, WUNTRACED))<0){perror("waitpid error");}
                tcsetpgrp(2,getpid());
            }else{//input,outputなし
                if(cur_process->next!=NULL){//input,outputなし, pipe
                    if(cur_process->next->output_redirection!=NULL){ //input,  pipe,output
                        pid_t pid1,pid2;
                        int wstatus1,wstatus2;
                        int fd[2];
                        int p;
                        if((p=pipe(fd))<0){perror("pipe error");}
                        if((pid1=fork())==0){ //パイプの受信側
                            struct sigaction newact;
                            sig_ign_drop(&newact);

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
                            struct sigaction newact;
                            sig_ign_drop(&newact);

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
                        tcsetpgrp(2,pid1);

                        int w2;
                        if ((w2 = waitpid(pid2,&wstatus2, WUNTRACED))<0){perror("waitpid error");}
                        int w1;
                        if ((w1 = waitpid(pid1,&wstatus1, WUNTRACED))<0){perror("waitpid error");}
                        tcsetpgrp(2,getpid());
                    }else{//input  pipe
                    pid_t pid1,pid2;
                    int wstatus1,wstatus2;
                    int fd[2];
                    int p;
                    if((p=pipe(fd))<0){perror("pipe error");}
                    if((pid1=fork())==0){ //パイプの受信側
                        struct sigaction newact;
                        sig_ign_drop(&newact);

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
                        struct sigaction newact;
                        sig_ign_drop(&newact);

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
                    tcsetpgrp(2,pid1);

                    int w2;
                    if ((w2 = waitpid(pid2,&wstatus2, WUNTRACED))<0){perror("waitpid error");}
                    int w1;
                    if ((w1 = waitpid(pid1,&wstatus1, WUNTRACED))<0){perror("waitpid error");}
                    }
                    tcsetpgrp(2,getpid());
                }else{//input,outputなし, pipeなし
                    pid_t pid;
                    int wstatus;
                    if((pid=fork())==0){//inputファイル受信側
                        struct sigaction newact;
                        sig_ign_drop(&newact);

                        int ifd;
                        if((ifd=open(cur_process->input_redirection,O_RDONLY))<0){perror("open error");}
                        dup2(ifd,0);
                        int ret = execve(cur_process->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    setpgid(pid,0);
                    tcsetpgrp(2,pid);
                    int w;
                    if ((w = waitpid(pid,&wstatus, WUNTRACED))<0){perror("waitpid error");}
                    tcsetpgrp(2,getpid());
                }
            }
        }else{
            if(cur_process->output_redirection!=NULL){
                pid_t pid;
                int wstatus;
                if((pid=fork())==0){
                    struct sigaction newact;
                    sig_ign_drop(&newact);

                    int ofd;
                    if((ofd=open(cur_process->output_redirection,O_WRONLY|O_CREAT,S_IRWXU))<0){perror("open error");}
                    dup2(ofd,1);
                    int ret = execve(cur_process->program_name,newargv,envp);
                    if (ret < 0){perror("execve error");}
                }
                setpgid(pid,0);
                tcsetpgrp(2,pid);
                int w;
                if ((w = waitpid(pid,&wstatus, WUNTRACED))<0){perror("waitpid error");}
                tcsetpgrp(2,getpid());
            }else{
                if(cur_process->next!=NULL){//pipe
                    if(cur_process->next->output_redirection!=NULL){//pipe,output
                        pid_t pid1,pid2;
                        int wstatus1,wstatus2;
                        int fd[2];
                        int p;
                        if((p=pipe(fd))<0){perror("pipe error");}
                        if((pid1=fork())==0){ //パイプの受信側
                            //struct sigaction newact;
                            //sig_ign_drop(&newact);

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
                            //struct sigaction newact;
                            //sig_ign_drop(&newact);

                            dup2(fd[1],1);
                            close(fd[0]);
                            int ret = execve(cur_process->program_name,newargv,envp);
                            if (ret < 0){perror("execve error");}
                        }
                        close(fd[0]);
                        close(fd[1]);

                        setpgid(pid1,0);
                        setpgid(pid2,pid1);
                        tcsetpgrp(2,pid1);

                        int w2;
                        if ((w2 = waitpid(pid2,&wstatus2, WUNTRACED))<0){perror("waitpid error");}
                        int w1;
                        if ((w1 = waitpid(pid1,&wstatus1, WUNTRACED))<0){perror("waitpid error");}
                        //tcsetpgrp(2,getpid());
                    }else{//pipe
                        pid_t pid1,pid2;
                        int wstatus1,wstatus2;
                        int fd[2];
                        int p;
                        if((p=pipe(fd))<0){perror("pipe error");}
                        if((pid1=fork())==0){ //パイプの受信側
                            //setpgid(getpid(),0);
                            //tcsetpgrp(2,getpid());
                            struct sigaction newact;
                            sig_ign_drop(&newact);

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
                            struct sigaction newact;
                            sig_ign_drop(&newact);

                            dup2(fd[1],1);
                            close(fd[0]);
                            int ret = execve(cur_process->program_name,newargv,envp);
                            if (ret < 0){perror("execve error");}
                        }
                        close(fd[0]);
                        close(fd[1]);

                        setpgid(pid1,0);
                        setpgid(pid2,pid1);
                        tcsetpgrp(2,pid1);

                        int w2;
                        if ((w2 = waitpid(pid2,&wstatus2, WUNTRACED))<0){perror("waitpid error");}
                        int w1;
                        if ((w1 = waitpid(pid1,&wstatus1, WUNTRACED))<0){perror("waitpid error");}
                        tcsetpgrp(2,getpid());
                    }
                }else{//
                    pid_t pid;
                    int wstatus;
                    if((pid=fork())==0){
                        struct sigaction newact;
                        sig_ign_drop(&newact);
                        
                        int ret = execve(cur_process->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }else{
                        if(curr_job->mode==BACKGROUND){
                            int s;
                            if((s = sigaction(SIGCHLD,&bgact,NULL))<0){perror("sigaction error");exit(1);}
                            setpgid(pid,0);
                            tcsetpgrp(2,getpid());
                        }else{
                            setpgid(pid,0);
                            tcsetpgrp(2,pid);
                            //pid_t pid2;
                            //int tty = isatty(0);
                            //printf("%d\n",tty);
                            //pid2 = tcgetpgrp(1);
                            //printf("%d\n",pid2);

                            int w;
                            if ((w = waitpid(pid,&wstatus, WUNTRACED))<0){perror("waitpid error");}
                            tcsetpgrp(2,getpid());
                            //printf("%d\n",getpid());
                            //pid2 = tcgetpgrp(0);
                            //printf("%d\n",pid2);
                        }
                    }
                }
            }
        }



       // print_job_list(curr_job);

        free_job(curr_job);
    }

    return 0;
}
