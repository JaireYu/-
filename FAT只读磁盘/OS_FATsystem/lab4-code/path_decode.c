#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "fat16.h"
/** TODO:
 * 将FAT文件名格式解码成原始的文件名
 * 
 * Hint:假设path是“FILE    TXT”，则返回"file.txt"
**/
/**  
  char s[][32] = {"..        ", "FILE    TXT", "ABCD    RM "};
  char sr[][32] = {"..", "file.txt", "abcd.rm"};
**/
BYTE *path_decode(BYTE *path)  //BYTE char
{
    BYTE *pathDecoded = (BYTE *)malloc(MAX_SHORT_NAME_LEN * sizeof(BYTE));
    memset((void *)pathDecoded, 0, MAX_SHORT_NAME_LEN);
    int i, end, dot;
    if(path[8] != 32){ //不是文件是目录
        for(i = 7; path[i] == ' ' && i != -1; i --){
        }
        end = i;
        for(i = 0; i != end + 1; i++){
            if(65 <= path[i] && path[i] <= 90){
                pathDecoded[i] = path[i] + 32;
            }
            else {
                pathDecoded[i] = path[i];
            }
        }
        dot = end + 1;
        pathDecoded[dot] = '.';
        for(i = 10; path[i] == ' ' && i != 7; i --){
        }
        end = i;
        for(i = 8; i != end + 1; i ++){
            if(65 <= path[i] && path[i] <= 90){
                pathDecoded[i-7+dot] = path[i] + 32;
            }
            else {
                pathDecoded[i-7+dot] = path[i];
            }
        }
    }
    else{  //是目录不是文件
        for(i = 7; path[i] == ' ' && i != -1; i --){
        }
        end = i;
        for(i = 0; i != end + 1; i++){
            if(65 <= path[i] && path[i] <= 90){
                pathDecoded[i] = path[i] + 32;
            }
            else {
                pathDecoded[i] = path[i];
            }
        }
    }
    return pathDecoded;
}