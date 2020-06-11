#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "parse.h"
#define NSTACK 100

volatile int waiting_pid;
pid_t pid_stack[NSTACK];
int count;

void print_job_list(job*);

void sig_ign(struct sigaction*);

void sig_ign_drop(struct sigaction*);

void wait_child(int ,siginfo_t* ,void*);

void sig_wait_child(struct sigaction*);

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
    if((sig = sigaction(SIGINT,&act,NULL))<0){perror("sigaction error");exit(1);}
    if((sig = sigaction(SIGTTOU,&act,NULL))<0){perror("sigaction error");exit(1);}
    if((sig = sigaction(SIGTSTP,&act,NULL))<0){perror("sigaction error");exit(1);}
    if((sig = sigaction(SIGCHLD,&bgact,NULL))<0){perror("sigaction error");exit(1);}
    //Ignore signals:SIGINT,SIGTTOU,SIGTSTP
    //Set handler for SIGCHLD

    while(get_line(s, LINELEN)) {
        if(!strcmp(s, "exit\n")){
            break;
        }
        if(!strcmp(s,"bg\n")){
            kill(pid_stack[count-1],SIGCONT);
            count--;
        }else if(strcmp(s,"\n")){

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
                        if((pid1=fork())==0){ //pipe receiver 
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
                        if((pid2=fork())==0){//pipe sender
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
                        if((pid1=fork())==0){ //pipe receiver 
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
                        if((pid2=fork())==0){//pipe sender
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
                        if((pid1=fork())==0){ //pipe receiver
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
                        if((pid2=fork())==0){//pipe sender
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
                        if((pid1=fork())==0){ //pipe receiver
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
                        if((pid2=fork())==0){//pipe sender
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
                        if((pid=fork())==0){
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
                        if((pid=fork())==0){//
                            if((sig = sigaction(SIGINT,&newact,NULL))<0){perror("sigaction error");exit(1);}

                            int ifd;
                            if((ifd=open(cur_process->input_redirection,O_RDONLY))<0){perror("open error");}
                            if(errno==EINTR){
                                if((ifd=open(cur_process->input_redirection,O_RDONLY))<0){perror("open error");}
                            }
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
                            if((sig = sigaction(SIGTSTP,&newact,NULL))<0){perror("sigaction error");exit(1);}
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

            if((sig = sigaction(SIGINT,&act,NULL))<0){perror("sigaction error");exit(1);}
            if((sig = sigaction(SIGTTOU,&act,NULL))<0){perror("sigaction error");exit(1);}
            if((sig = sigaction(SIGTSTP,&act,NULL))<0){perror("sigaction error");exit(1);}
            if((sig = sigaction(SIGCHLD,&bgact,NULL))<0){perror("sigaction error");exit(1);}

        // print_job_list(curr_job);

            free_job(curr_job);
            }
    }

    return 0;
}
