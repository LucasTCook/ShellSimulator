//
//  main.c
//  SHELL
//
//  Created by Lucas Cook on 9/23/17.
//  Sources: TutorialPoint.com, StackOverflow.com
//          https://brennan.io/2015/01/16/write-a-shell-in-c/
//          https://www.cs.purdue.edu/homes/grr/SystemsProgrammingBook/Book/Chapter5-WritingYourOwnShell.pdf

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

//Define the Max length of a command and the mac number of args
#define MAX_LENGTH 80
#define MAX_ARGS 10
#define HIST_SIZE 10

struct history{
    int id;
    char * cmds[HIST_SIZE];	//Previous Commands
    int size;	//Number of Commands
};

static struct history cmdHistory = {0, NULL, 0};

static void showHistoryHandler(const int signal){
    if(signal == SIGINT){
        printf("\n");
        
        //Check to see if the history buffer contains any commands
        if(cmdHistory.size > 0){
            int i;
            //Print out up to the past 10 commands
            for(i=0; i < cmdHistory.size; i++){
//                printf("ID: %i\n", cmdHistory.id);
//                printf("SIZE: %i\n", cmdHistory.size);
                printf("%i %s\n", cmdHistory.id - cmdHistory.size + i + 1, cmdHistory.cmds[i]);
            }
        }else{
            printf("(empty)\n");
        }
    }
};


static void addToCmdHistory(const char * cmd){
    //If the History buffer is full then shift
    if(cmdHistory.size == HIST_SIZE){
        free(cmdHistory.cmds[0]);
        for(int i = 1; i< HIST_SIZE; i++){
            cmdHistory.cmds[i-1] = cmdHistory.cmds[i];
        }
        cmdHistory.size--;
    }
    cmdHistory.id++;
    cmdHistory.cmds[cmdHistory.size++] = strdup(cmd);
}

static int copyFromCmdHist(const int id, char * cmdLine){
    int x;
    
    
    //Get the index of the history item selected
    if(id == -1)
        x = cmdHistory.id - cmdHistory.size;
    else
        x = cmdHistory.id - cmdHistory.size + id -1;
    
    printf("COPYING %i\n", x);
    //Copy the content of that index if it is valid
    if((x>=0) && (x < cmdHistory.size)){
        strncpy(cmdLine, cmdHistory.cmds[x], MAX_LENGTH);
    }else{
        printf("Error: Invalid history index!\n");
        return 1;
    }
    return 0;
}


static int getCmd(char cmd[]){
    int length;
    printf("sh>");
    
    //If no command was entered then print "sh>" again
    //If CTRL->D is entered then return 1
    //If a command is entered then return 0
    while(fgets(cmd, MAX_LENGTH, stdin) == NULL){
        if(feof(stdin)){
            printf("\n");
            return 1;
        }
        printf("sh> ");
    }
    
    //Get the length of the command entered
    length = strlen(cmd);
    
    //Have to set the last element of the array to '/0'
    cmd[length-1] = '\0';
    
    return 0;
}


// Split the arguments by the spaces
static int parseInput(char * cmd, char *args[]){
    int i = 0;
    int isBackground = 0;
    
    args[i] = strtok(cmd, " \t");
    
    while(++i < MAX_ARGS){
        
        //Separates the arguments by '\t'
        args[i] = strtok(NULL, " \t");
        if(args[i] == NULL)
            break;
        
        if(args[i][0] == '&'){
            isBackground = 1;
        }
    }
    return isBackground;
}

static void run(char ** args, const int background){
    
    //STEP 3: CREATE A NEW PROCESS SPACE FOR THE NEW COMMAND (CHILD)
    pid_t pid = fork();
    switch(pid){
        case -1:
            exit(1);
            break;
            
            
            //If the pid indicates a child process, then execute
        case 0:
            //STEP 4: EXECUTE THE COMMAND
            execvp(args[0], args);
            break;
            
            
            
            //By default if no other conditions are met, then wait (PARENT)
        default:
            if(background == 0){
                int status;
                waitpid(pid, &status, 0); //wait
            }
            break;
    }
}

int main(void){
    
    //showHistoryHandler (CTRL-C)
    struct sigaction action, action_old;
    action.sa_handler = showHistoryHandler;
    action.sa_flags = 0;
    sigemptyset(&action.sa_mask);
    if(sigaction(SIGINT,  &action, &action_old) == -1){
        perror("sigaction");
        return 1;
    }
    
    
    
    //Buffer to hold the command command
    char cmd[MAX_LENGTH];
    
    
    char *args[MAX_ARGS+1];
    
    while(1){
        
        bzero(cmd, MAX_LENGTH * sizeof(char));
        bzero(args, sizeof(char*)*(MAX_ARGS+1));
        
        
        //STEP 1: GET THE COMMAND
        //if Ctrl->D is pressed
        if(getCmd(cmd) == 1)
            break;
        
        int id = -1;
        if(strncmp(cmd, "r ", 2) == 0){
            id = cmd[2] - '0';
        }
        else if(strncmp(cmd, "r" , 1) == 0)
        {
            id = -1;
        }
        if(id > -1){
            if(copyFromCmdHist(id, cmd) == 1)
                continue;
        }
        
        addToCmdHistory(cmd);	//save in history
        
        
        /*The parse_input function separates the arguments entered and checks to see if there is a & following the command, if it is then it returns the value 1. */
        int isBackground = parseInput(cmd, args);
        
        
        //If the args is empty then continue
        if(args[0] == NULL){
            continue;
        }
        
        //STEP 2: RUN THE COMMAND
        run(args, isBackground);
    }
    
    for(int i = 0; i < cmdHistory.size; i++)
        free(cmdHistory.cmds[i]);
    
    return 0;
}
