#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "fat16.h"
char **path_split(char *pathInput, int *pathDepth_ret)
{
    int pathDepth = 0;
    int i = 0, j = 0, k = 0, start = 0, end = 0, dot = 0;
    bool havedot;
    while (pathInput[i] != '\0') {
        if (pathInput[i] == '/')
            pathDepth++;
        i++;
    }
    char **paths = (char **)malloc(pathDepth * sizeof(char *));
    for(j = start; j != '\0' && j != '/'; j ++){
    }
    start = j;
    for(i = 0; i != pathDepth; i ++){
        paths[i] = (char *)malloc((13-1) * sizeof(char));
        memset((void *)paths[i], 0, 13-1);
        for(j = start + 1; pathInput[j] != '/' && pathInput[j] != '\0'; j++){}
        end = j;
        havedot = false;
        for(j = start + 1; j != end; j++){
            if(pathInput[j] == '.'){
                havedot = true;
                dot = j;
            }
        }
        if(havedot){  //是文件不是目录
            if((dot-start-1) > 8){  //文件名超过长度范围
                for(j = start + 1; j != start + 9; j++){
                    if(97 <= pathInput[j] && pathInput[j]<= 122){//小写字母
                        paths[i][j -start-1] = pathInput[j] - 32;
                    }
                    else{   //其他
                        paths[i][j -start-1] = pathInput[j];
                    }
                }
            }
            else{//文件名没有超过长度范围
                for(j = start + 1; j != dot; j++){
                    if(97 <= pathInput[j] && pathInput[j]<= 122){//小写字母
                        paths[i][j -start-1] = pathInput[j] - 32;
                    }
                    else{   //其他
                        paths[i][j -start-1] = pathInput[j];
                    }
                }
                for(j = dot; j != start + 9; j ++){
                    paths[i][j -start-1] = 32;
                }
            }
            if((end - dot - 1) > 3){ //扩展名超过范围
                for(j = dot + 1; j != dot + 4; j++){
                    if(97 <= pathInput[j] && pathInput[j]<= 122){//小写字母
                        paths[i][j +7 -dot] = pathInput[j] - 32;
                    }
                    else{   //其他
                        paths[i][j +7 -dot] = pathInput[j];
                    }
                }
            }
            else{
                for(j = dot + 1; j != end; j++){
                    if(97 <= pathInput[j] && pathInput[j]<= 122){//小写字母
                        paths[i][j +7 -dot] = pathInput[j] - 32;
                    }
                    else{   //其他
                        paths[i][j +7-dot] = pathInput[j];
                    }
                }
                for(j = end; j != dot + 4; j ++){
                    paths[i][j +7-dot] = 32;
                }
            }
        }
        else{   //是目录不是文件
            if((end - start -1)>8){ //目录名字超过范围
                for(j = start + 1; j != start + 9; j++){
                    if(97 <= pathInput[j] && pathInput[j]<= 122){//小写字母
                        paths[i][j -start-1] = pathInput[j] - 32;
                    }
                    else{   //其他
                        paths[i][j -start-1] = pathInput[j];
                    }
                }
                for(j = start + 9; j != start + 12; j ++){
                    paths[i][j -start-1] = 32;
                }
            }
            else{  //目录名字没有超过范围
                for(j = start + 1; j != end; j ++){
                    if(97 <= pathInput[j] && pathInput[j]<= 122){//小写字母
                        paths[i][j -start-1] = pathInput[j] - 32;
                    }
                    else{   //其他
                        paths[i][j -start-1] = pathInput[j];
                    }
                }
                for(j = end; j != start + 12; j ++){
                    paths[i][j -start-1] = 32;
                }
            }
        }
        start = end;
    }
    *pathDepth_ret = pathDepth;
    return paths;
}