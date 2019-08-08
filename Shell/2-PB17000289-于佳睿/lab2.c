#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/syscall.h>
#include<sys/wait.h>
void sepspace(char* str, char **cmd){
    int i = 0;
    cmd[0] = strtok(str, " ");
    for(i = 1; i < 10; i ++){
        cmd[i] = strtok(NULL, " ");
    }
}
int sepsemicolon(char *cmdline, char **cmd){
    int i = 0;
    int num = 0;
    cmd[0] = strtok(cmdline, ";");
    num = num + 1;
    for(i = 1; i < 10; i++){
        cmd[i] = strtok(NULL, ";");
        if(cmd[i] != NULL)
            num = num + 1;
    }
    return num;
}
void seppipe(char* str, char **cmd){
    int i = 0;
    cmd[0] = strtok(str, "|");
    cmd[1] = strtok(NULL, "|");
}
int main(){
    char cmdline[256];
    while (1) {
        printf("OSLab2->");
        gets(cmdline);
        int cmd_num = 0;
        char *cmd[10];
        int i = 0;
        int j = 0;
        int k = 0;
        cmd_num = sepsemicolon(cmdline, cmd);
        for (i=0; i < cmd_num; i++) {
            int pipe_index = 0;
            int pipe = 0;
            for(j = 0; j != strlen(cmd[i]); j ++){
                if(cmd[i][j] == '|'){
                    pipe = 1;
                    break;
                }
            }
            if (pipe) { //处理包含一个管道符号“|”的情况,利用popen处理命令的输入输出转换
                char *pipe_cmd[2];
                seppipe(cmd[i], pipe_cmd);
                char buffer[10000];
                FILE *fr = popen(pipe_cmd[0], "r");
                FILE *fw = popen(pipe_cmd[1], "w");
                if(fr != NULL && fw != NULL){
                    int successnum = 0;
                    successnum = fread(buffer, sizeof(char), 10000, fr);
                    if(successnum == 10000){
                        printf("Buffer overflow...Cancel the command\n");
                        pclose(fr);
                        pclose(fw);
                    }
                    else if(fw != NULL){
                        fwrite(buffer, sizeof(char), 1000, fw);
                        pclose(fr);
                        pclose(fw);
                    }
                }
                else{
                    printf("right or left command error!\n");
                    if(fr != NULL)
                        pclose(fr);
                    if(fw != NULL)
                        pclose(fw);
                }
            }
            else {
                char *cmd_nopipe[10];
                int count = 0;
                sepspace(cmd[i], cmd_nopipe);
                pid_t child_pid = fork();
                if(child_pid == -1){
                    printf("Failed to fork a child process\n");
                }
                else if(child_pid == 0){
                    //child_process
                    if(execvp(cmd_nopipe[0], cmd_nopipe)<0)
                        printf("Cannot execute the %d command\n", i);
                    exit(0);
                }
                else{
                    //parent_process
                    int status;
                    status = waitpid(child_pid, &status, 0);
                    if(status == 0)
                        printf("Failed to execute child process\n");
                    else if(status == -1)
                        printf("There are errors while executing child process\n");
                    else if(status != child_pid)
                        printf("Executing wrong child...please try again\n");
                }
            }

        }
    }

    return 0;
}