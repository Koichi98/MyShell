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

int main(int argc, char *argv[],char *envp[]) {
    char s[LINELEN];
    job *curr_job;

    while(get_line(s, LINELEN)) {
        if(!strcmp(s, "exit\n")){
            printf("hello");
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
                    int ifd;
                    if((ifd=open(cur_process->input_redirection,O_RDONLY))<0){perror("open error");}
                    dup2(ifd,0);
                    int ofd;
                    if((ofd=open(cur_process->output_redirection,O_WRONLY|O_CREAT,S_IRWXU))<0){perror("open error");}
                    dup2(ofd,1);
                    int ret = execve(cur_process->program_name,newargv,envp);
                    if (ret < 0){perror("execve error");}
                }
                int w;
                if ((w = waitpid(pid,&wstatus, WUNTRACED))<0){perror("waitpid error");}
            }else{//input,outputなし
                if(cur_process->next!=NULL){//input,outputなし, pipe
                    if(cur_process->next->output_redirection!=NULL){ //input,  pipe,output
                        pid_t pid1,pid2;
                        int wstatus1,wstatus2;
                        int fd[2];
                        int p;
                        if((p=pipe(fd))<0){perror("pipe error");}
                        if((pid1=fork())==0){ //パイプの受信側
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
                        int w2;
                        if ((w2 = waitpid(pid2,&wstatus2, WUNTRACED))<0){perror("waitpid error");}
                        int w1;
                        if ((w1 = waitpid(pid1,&wstatus1, WUNTRACED))<0){perror("waitpid error");}
                    }else{//input  pipe
                    pid_t pid1,pid2;
                    int wstatus1,wstatus2;
                    int fd[2];
                    int p;
                    if((p=pipe(fd))<0){perror("pipe error");}
                    if((pid1=fork())==0){ //パイプの受信側
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
                    int w2;
                    if ((w2 = waitpid(pid2,&wstatus2, WUNTRACED))<0){perror("waitpid error");}
                    int w1;
                    if ((w1 = waitpid(pid1,&wstatus1, WUNTRACED))<0){perror("waitpid error");}
                    }
                }else{//input,outputなし, pipeなし
                    pid_t pid;
                    int wstatus;
                    if((pid=fork())==0){//inputファイル受信側
                        int ifd;
                        if((ifd=open(cur_process->input_redirection,O_RDONLY))<0){perror("open error");}
                        dup2(ifd,0);
                        int ret = execve(cur_process->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }
                    int w;
                    if ((w = waitpid(pid,&wstatus, WUNTRACED))<0){perror("waitpid error");}
                }
            }
        }else{
            if(cur_process->output_redirection!=NULL){
                pid_t pid;
                int wstatus;
                if((pid=fork())==0){
                    int ofd;
                    if((ofd=open(cur_process->output_redirection,O_WRONLY|O_CREAT,S_IRWXU))<0){perror("open error");}
                    dup2(ofd,1);
                    int ret = execve(cur_process->program_name,newargv,envp);
                    if (ret < 0){perror("execve error");}
                }
                int w;
                if ((w = waitpid(pid,&wstatus, WUNTRACED))<0){perror("waitpid error");}
            }else{
                if(cur_process->next!=NULL){//pipe
                    if(cur_process->next->output_redirection!=NULL){//pipe,output
                        pid_t pid1,pid2;
                        int wstatus1,wstatus2;
                        int fd[2];
                        int p;
                        if((p=pipe(fd))<0){perror("pipe error");}
                        if((pid1=fork())==0){ //パイプの受信側
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
                            dup2(fd[1],1);
                            close(fd[0]);
                            int ret = execve(cur_process->program_name,newargv,envp);
                            if (ret < 0){perror("execve error");}
                        }
                        close(fd[0]);
                        close(fd[1]);
                        int w2;
                        if ((w2 = waitpid(pid2,&wstatus2, WUNTRACED))<0){perror("waitpid2 error");}
                        int w1;
                        if ((w1 = waitpid(pid1,&wstatus1, WUNTRACED))<0){perror("waitpid1 error");}
                    }else{//pipe
                        pid_t pid1,pid2;
                        int wstatus1,wstatus2;
                        int fd[2];
                        int p;
                        if((p=pipe(fd))<0){perror("pipe error");}
                        if((pid1=fork())==0){ //パイプの受信側
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
                            dup2(fd[1],1);
                            close(fd[0]);
                            int ret = execve(cur_process->program_name,newargv,envp);
                            if (ret < 0){perror("execve error");}
                        }
                        close(fd[0]);
                        close(fd[1]);
                        int w2;
                        if ((w2 = waitpid(pid2,&wstatus2, WUNTRACED))<0){perror("waitpid2 error");}
                        int w1;
                        if ((w1 = waitpid(pid1,&wstatus1, WUNTRACED))<0){perror("waitpid1 error");}
                    }
                }else{//
                    pid_t pid;
                    int wstatus;
                    if((pid=fork())==0){
                        int ret = execve(cur_process->program_name,newargv,envp);
                        if (ret < 0){perror("execve error");}
                    }else{
                        int w;
                        if ((w = waitpid(pid,&wstatus, WUNTRACED))<0){perror("waitpid error");}
                    }
                }
            }
        }


        //if(cur_process->input_redirection!=NULL){
            //int fd1[2],fd2[2];
            //int p1,p2;
            //if((p1=pipe(fd1))<0){perror("pipe error");}
            //if((p2=pipe(fd2))<0){perror("pipe error");}
            //if((pid1=fork())==0){//inputファイル受信側,outputファイル送信側
                //dup2(fd1[0],0);
                //close(fd1[1]);
                //if(cur_process->output_redirection!=NULL){
                    //dup2(fd2[1],1);
                    //close(fd2[0]);
                //}
                //int ret = execve(cur_process->program_name,newargv,envp);
                //if (ret < 0){perror("execve error");}
            //}
            //if((pid2=fork())==0){ //inputファイル送信側
                //dup2(fd1[1],1);
                //close(fd1[0]);
                //char buf[512];
                //int count = 512;
                //int ifd;
                //if((ifd=open(cur_process->input_redirection,O_RDONLY))<0){perror("open error");}
                //int wsize,rsize;
                //if ((rsize=read(ifd,buf,count))<0){perror("read error");}
                //while(rsize>0){
                    //if ((wsize=write(fd1[1],buf,rsize))<0){perror("write error");}
                    //if ((rsize=read(ifd,buf,count))<0){perror("read error");}
                //}
                //close(ifd);
                //close(fd1[1]);
                //return 0;
            //}
            //if(cur_process->output_redirection!=NULL){
                //if((pid3=fork())==0){//outputファイル受信側
                    //dup2(fd2[0],0);
                    //close(fd2[1]);
                    //char buf[512];
                    //int count = 512;
                    //int rsize;
                    //if ((rsize=read(fd2[0],buf,count))<0){perror("read error");}
                    //int ofd;
                    //if((ofd=open(cur_process->output_redirection,O_WRONLY|O_CREAT,S_IRWXU))<0){perror("open error");}
                    //int wsize;
                    //while(rsize>0){
                        //if ((wsize=write(ofd,buf,rsize))<0){perror("write error");}
                        //if ((rsize=read(fd2[0],buf,count))<0){perror("read error");}
                    //}
                    //close(ofd);
                    //close(fd2[0]);
                    //return 0;
                //}
            //}
            //close(fd1[0]);
            //close(fd1[1]);
            //close(fd2[0]);
            //close(fd2[1]);
            //int w2;
            //if ((w2 = waitpid(pid2,&wstatus2, WUNTRACED))<0){perror("waitpid error");}
            //int w1;
            //if ((w1 = waitpid(pid1,&wstatus1, WUNTRACED))<0){perror("waitpid error");}
            //if(cur_process->output_redirection!=NULL){
            //int w3;
            //if ((w3 = waitpid(pid3,&wstatus3, WUNTRACED))<0){perror("waitpid error");}
            //}
        //}
        //if(cur_process->output_redirection!=NULL){
            //int fd[2];
            //int p;
            //if((p=pipe(fd))<0){perror("pipe error");}
            //if((pid=fork())==0){
                //dup2(fd[1],1);
                //close(fd[0]);
                //int ret = execve(cur_process->program_name,newargv,envp);
                //if (ret < 0){perror("execve error");}
            //}else{
                //dup2(fd[0],0);
                //close(fd[1]);
                //char* buf[512];
                //int count = 512;
                //int r;
                //if ((r=read(fd[0],buf,count))<0){perror("read error");}
                //int fd1;
                //if((fd1=open(cur_process->output_redirection,O_WRONLY|O_CREAT,S_IRWXU))<0){perror("open error");}
                //int wsize;
                //while(r!=0){
                     //if ((wsize=write(fd1,buf,count))<0){perror("write error");}
                     //if ((r=read(fd[0],buf,count))<0){perror("read error");}
                //}
                //int w;
                //if ((w = waitpid(pid,&wstatus, WUNTRACED))<0){perror("waitpid error");}
                //close(fd1);
                //close(fd[0]);
            //}


        print_job_list(curr_job);

        free_job(curr_job);
    }

    return 0;
}
