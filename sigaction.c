#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#define NSTACK 100

volatile int waiting_pid;
pid_t pid_stack[NSTACK];
int count=0;

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
    act->sa_handler = SIG_DFL;
    sigset_t sigset;
    int sigem;
    if((sigem=sigemptyset(&sigset))<0){perror("sigemptyset error");exit(1);}
    act->sa_mask=sigset;
    act->sa_flags=0;
    act->sa_restorer=NULL;
}

void wait_child(int signal,siginfo_t* info,void* ctx){
    if(info->si_code==CLD_STOPPED){
        pid_stack[count] = info->si_pid;
        count++;
    }
    if(info->si_code != CLD_CONTINUED){
    int wstatus;
    if ((waiting_pid = waitpid(info->si_pid,&wstatus, WUNTRACED))<0){perror("waitchild error");}
    }
    //printf("accepted");   //For debug(NOT RECOMMENDED)
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

