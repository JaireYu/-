#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "fat16.h"

char *FAT_FILE_NAME = "fat16.img";

/* 将扇区号为secnum的扇区读到buffer中 */
void sector_read(FILE *fd, unsigned int secnum, void *buffer)
{
    fseek(fd, BYTES_PER_SECTOR * secnum, SEEK_SET);
    fread(buffer, BYTES_PER_SECTOR, 1, fd);
}

/** TODO:FINISHED
 * 将输入路径按“/”分割成多个字符串，并按照FAT文件名格式转换字符串
 *
 * Hint1:假设pathInput为“/dir1/dir2/file.txt”，则将其分割成“dir1”，“dir2”，“file.txt”，
 *      每个字符串转换成长度为11的FAT格式的文件名，如“file.txt”转换成“FILE    TXT”，
 *      返回转换后的字符串数组，并将*pathDepth_ret设置为3
 * Hint2:可能会出现过长的字符串输入，如“/.Trash-1000”，需要自行截断字符串//未解决
**/
char **path_split(char *pathInput, int *pathDepth_ret)
{
    int pathDepth = 0;
    int i = 0, j = 0, k = 0, start = 0, end = 0, dot = 0;
    int havedot;
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
        havedot = 0;
        for(j = start + 1; j != end; j++){
            if(pathInput[j] == '.'){
                havedot = 1;
                dot = j;
            }
        }
        if(havedot == 1){  //是文件不是目录
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

/** TODO:FINISHED
 * 将FAT文件名格式解码成原始的文件名
 *
 * Hint:假设path是“FILE    TXT”，则返回"file.txt"
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
/*下面是四个自定义函数*/
DWORD get_DWORD(FILE *fp){
    BYTE *s;
    DWORD i;
    s = (BYTE *)&i;
    s[0]=getc(fp);
    s[1]=getc(fp);
    s[2]=getc(fp);
    s[3]=getc(fp);
    return i;
}
WORD get_WORD(FILE *fp){
    BYTE *s;
    WORD i;
    s = (BYTE *)&i;
    s[0] =getc(fp);
    s[1]=getc(fp);
    return i;
}
DWORD CharIntoDWORD(BYTE *buffer){
    DWORD i;
    BYTE *s = (BYTE *)&i;
    s[0] = *buffer;
    s[1] = *(buffer + 1);
    s[2] = *(buffer + 2);
    s[3] = *(buffer + 3);
    return i;
}
WORD CharIntoWORD(BYTE * buffer){
    WORD i;
    BYTE *s = (BYTE *)&i;
    s[0] = *buffer;
    s[1] = *(buffer + 1);
    return i;
}

FAT16 *pre_init_fat16(void){
    /* Opening the FAT16 image file */
    FILE *fd;
    FAT16 *fat16_ins;

    fd = fopen(FAT_FILE_NAME, "rb");

    if (fd == NULL)
    {
        fprintf(stderr, "Missing FAT16 image file!\n");
        exit(EXIT_FAILURE);
    }

    fat16_ins = (FAT16 *)malloc(sizeof(FAT16));
    fat16_ins->fd = fd;

/** TODO: FINISHED
   * 初始化fat16_ins的其余成员变量
   * Hint: root directory的大小与Bpb.BPB_RootEntCnt有关，并且是扇区对齐的
**/
    fread((char *)fat16_ins->Bpb.BS_jmpBoot, 3,1, fd);
    fread((char *)fat16_ins->Bpb.BS_OEMName, 8,1, fd);
    fat16_ins->Bpb.BPB_BytsPerSec = get_WORD(fd);
    fat16_ins->Bpb.BPB_SecPerClus = (BYTE)fgetc(fd);
    fat16_ins->Bpb.BPB_RsvdSecCnt = get_WORD(fd);
    fat16_ins->Bpb.BPB_NumFATS = (BYTE)fgetc(fd);
    fat16_ins->Bpb.BPB_RootEntCnt = get_WORD(fd);
    fat16_ins->Bpb.BPB_TotSec16 = get_WORD(fd);
    fat16_ins->Bpb.BPB_Media = (BYTE)fgetc(fd);
    fat16_ins->Bpb.BPB_FATSz16 = get_WORD(fd);
    fat16_ins->Bpb.BPB_SecPerTrk = get_WORD(fd);
    fat16_ins->Bpb.BPB_NumHeads = get_WORD(fd);
    fat16_ins->Bpb.BPB_HiddSec = get_DWORD(fd);
    fat16_ins->Bpb.BPB_TotSec32 = get_DWORD(fd);
    fat16_ins->Bpb.BS_DrvNum = (BYTE)fgetc(fd);
    fat16_ins->Bpb.BS_Reserved1 = (BYTE)fgetc(fd);
    fat16_ins->Bpb.BS_BootSig = (BYTE)fgetc(fd);
    fat16_ins->Bpb.BS_VollID = get_DWORD(fd);
    fread((char *)fat16_ins->Bpb.BS_VollLab, 11,1, fd);
    fread((char *)fat16_ins->Bpb.BS_FilSysType, 8,1, fd);
    fread((char *)fat16_ins->Bpb.Reserved2, 448,1, fd);
    fat16_ins->Bpb.Signature_word = get_WORD(fd);
    fat16_ins->FirstRootDirSecNum = fat16_ins->Bpb.BPB_NumFATS * fat16_ins->Bpb.BPB_FATSz16 + fat16_ins->Bpb.BPB_RsvdSecCnt;
    fat16_ins->FirstDataSector = fat16_ins->FirstRootDirSecNum + fat16_ins->Bpb.BPB_RootEntCnt/16;
    return fat16_ins;
}
/** TODO: FINISHED
 * 返回簇号为ClusterN对应的FAT表项
**/
WORD fat_entry_by_cluster(FAT16 *fat16_ins, WORD ClusterN){
    BYTE sector_buffer[BYTES_PER_SECTOR];
    DWORD fat2_sector = (fat16_ins->Bpb.BPB_RsvdSecCnt + fat16_ins->Bpb.BPB_FATSz16);  //fat2开始扇区
    if(ClusterN < fat16_ins->Bpb.BPB_FATSz16 * fat16_ins->Bpb.BPB_BytsPerSec){
        sector_read(fat16_ins->fd, fat2_sector + ClusterN * 2/fat16_ins->Bpb.BPB_BytsPerSec, sector_buffer);
        return CharIntoWORD((BYTE *)(sector_buffer + (ClusterN * 2)%fat16_ins->Bpb.BPB_BytsPerSec));
    }
    else
        return 0xffff;
}

/**
 * 根据簇号ClusterN，获取其对应的第一个扇区的扇区号和数据，以及对应的FAT表项
**/
void first_sector_by_cluster(FAT16 *fat16_ins, WORD ClusterN, WORD *FatClusEntryVal, WORD *FirstSectorofCluster, BYTE *buffer)
{
    *FatClusEntryVal = fat_entry_by_cluster(fat16_ins, ClusterN);  //获取ClusterN对应的下一簇
    *FirstSectorofCluster = ((ClusterN - 2) * fat16_ins->Bpb.BPB_SecPerClus) + fat16_ins->FirstDataSector; //以2开始32扇区的数据块簇号对应的第一个扇区号
    sector_read(fat16_ins->fd, *FirstSectorofCluster, buffer);
}

/** TODO:FINISHED
 * 从root directory开始，查找path对应的文件或目录，找到返回0，没找到返回1，并将Dir填充为查找到的对应目录项
 *
 * Hint: 假设path是“/dir1/dir2/file”，则先在root directory中查找名为dir1的目录，
 *       然后在dir1中查找名为dir2的目录，最后在dir2中查找名为file的文件，找到则返回0，并且将file的目录项数据写入到参数Dir对应的DIR_ENTRY中
**/
int find_root(FAT16 *fat16_ins, DIR_ENTRY *Dir, const char *path)
{
    int pathDepth;
    char **paths = path_split((char *)path, &pathDepth);

    /* 先读取root directory */
    int i,j;
    BYTE buffer[BYTES_PER_SECTOR];
    FILE *fd = fopen(FAT_FILE_NAME, "rb");
    fat16_ins->fd = fd;

    /** TODO:
     * 查找名字为paths[0]的目录项，
     * 如果找到目录，则根据pathDepth判断是否需要调用find_subdir继续查找，
     *
     * !!注意root directory可能包含多个扇区
    **/
    for (i = 0; i != fat16_ins->Bpb.BPB_RootEntCnt/16; i++)
    {
        sector_read(fat16_ins->fd, fat16_ins->FirstRootDirSecNum + i, buffer); //读一个扇区进来
        for (j = 0; j != 16; j ++){
            if(strncmp((char *)(buffer + j*32), paths[0], 11) == 0){    //如果相等找到了
                if((BYTE)buffer[j * 32 + 11] == 0x10){ //目录
                    if(pathDepth != 1){       //深度不为1寻找子目录
                        strncpy((char *)Dir->DIR_Name, (char *)(buffer + j * 32), 11);
                        Dir->DIR_Attr = buffer[j * 32 + 11];
                        Dir->DIR_NTRes = buffer[j * 32 + 12];
                        Dir->DIR_CrtTimeTenth = buffer[j * 32 + 13];
                        Dir->DIR_CrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 14));
                        Dir->DIR_CrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 16));
                        Dir->DIR_LstAccDate = CharIntoWORD((BYTE *) (buffer + j*32 + 18));
                        Dir->DIR_FstClusHI = CharIntoWORD((BYTE *) (buffer + j * 32 + 20));
                        Dir->DIR_WrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 22));
                        Dir->DIR_WrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 24));
                        Dir->DIR_FstClusLO = CharIntoWORD((BYTE *)(buffer + j * 32 + 26));
                        Dir->DIR_FileSize = CharIntoDWORD((BYTE *)(buffer + j * 32 + 28));
                        return find_subdir(fat16_ins, Dir, paths, pathDepth, 1);
                    }
                    else{                //深度为1停止寻找
                        strncpy((char *)Dir->DIR_Name, (char *)(buffer + j * 32), 11);
                        Dir->DIR_Attr = buffer[j * 32 + 11];
                        Dir->DIR_NTRes = buffer[j * 32 + 12];
                        Dir->DIR_CrtTimeTenth = buffer[j * 32 + 13];
                        Dir->DIR_CrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 14));
                        Dir->DIR_CrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 16));
                        Dir->DIR_LstAccDate = CharIntoWORD((BYTE *) (buffer + j*32 + 18));
                        Dir->DIR_FstClusHI = CharIntoWORD((BYTE *) (buffer + j * 32 + 20));
                        Dir->DIR_WrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 22));
                        Dir->DIR_WrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 24));
                        Dir->DIR_FstClusLO = CharIntoWORD((BYTE *)(buffer + j * 32 + 26));
                        Dir->DIR_FileSize = CharIntoDWORD((BYTE *)(buffer + j * 32 + 28));
                        return 0;
                    }
                }
                else{
                    if(pathDepth!=1)   //如果深度不为1而且是文件
                        return 1;
                    strncpy((char *)Dir->DIR_Name, (char *)(buffer + j * 32), 11);  //深度是一找到了
                    Dir->DIR_Attr = buffer[j * 32 + 11];
                    Dir->DIR_NTRes = buffer[j * 32 + 12];
                    Dir->DIR_CrtTimeTenth = buffer[j * 32 + 13];
                    Dir->DIR_CrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 14));
                    Dir->DIR_CrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 16));
                    Dir->DIR_LstAccDate = CharIntoWORD((BYTE *) (buffer + j*32 + 18));
                    Dir->DIR_FstClusHI = CharIntoWORD((BYTE *) (buffer + j * 32 + 20));
                    Dir->DIR_WrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 22));
                    Dir->DIR_WrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 24));
                    Dir->DIR_FstClusLO = CharIntoWORD((BYTE *)(buffer + j * 32 + 26));
                    Dir->DIR_FileSize = CharIntoDWORD((BYTE *)(buffer + j * 32 + 28));
                    return 0;
                }
            }
        }
    }

    return 1;
}

/** TODO: FINISHED
 * 从子目录开始查找path对应的文件或目录，找到返回0，没找到返回1，并将Dir填充为查找到的对应目录项
 *
 * Hint1: 在find_subdir入口处，Dir应该是要查找的这一级目录的表项，需要根据其中的簇号，读取这级目录对应的扇区数据
 * Hint2: 目录的大小是未知的，可能跨越多个扇区或跨越多个簇；当查找到某表项以0x00开头就可以停止查找
 * Hint3: 需要查找名字为paths[curDepth]的文件或目录，同样需要根据pathDepth判断是否继续调用find_subdir函数
**/
int find_subdir(FAT16 *fat16_ins, DIR_ENTRY *Dir, char **paths, int pathDepth, int curDepth)
{
    int i, j;
    int DirSecCnt = 1;  /* 用于统计已读取的扇区数 */
    FILE* fd = fopen(FAT_FILE_NAME, "rb");
    fat16_ins->fd = fd;
    BYTE buffer[BYTES_PER_SECTOR];
    WORD ClusterN;         //索引簇号
    WORD FatClusEntryVal;  //下一簇号
    WORD FirstSectorofCluster; //数据区中该簇的首扇区号
    ClusterN = Dir->DIR_FstClusLO;  //从Dir中读取索引簇号

    while(ClusterN != 0xffff){    //知道簇号为0xffff
        first_sector_by_cluster(fat16_ins, ClusterN, &FatClusEntryVal, &FirstSectorofCluster, buffer); //读第一个扇区
        ClusterN = FatClusEntryVal;
        for(j = 0; j != 16; j ++){ //处理第一个扇区的16个条目
            if(buffer[j*32] == 0x00)    //如果开始为0，没有记录就结束
                return 1;
            else if(strncmp((char *)(buffer + j*32), paths[curDepth], 11) == 0){ //找到了
                if((BYTE)buffer[j * 32 + 11] == 0x10){ //目录
                    if(pathDepth != (curDepth + 1)){   //没有到底
                        strncpy((char *)Dir->DIR_Name, (char *)(buffer + j * 32), 11);
                        Dir->DIR_Attr = buffer[j * 32 + 11];
                        Dir->DIR_NTRes = buffer[j * 32 + 12];
                        Dir->DIR_CrtTimeTenth = buffer[j * 32 + 13];
                        Dir->DIR_CrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 14));
                        Dir->DIR_CrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 16));
                        Dir->DIR_LstAccDate = CharIntoWORD((BYTE *) (buffer + j*32 + 18));
                        Dir->DIR_FstClusHI = CharIntoWORD((BYTE *) (buffer + j * 32 + 20));
                        Dir->DIR_WrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 22));
                        Dir->DIR_WrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 24));
                        Dir->DIR_FstClusLO = CharIntoWORD((BYTE *)(buffer + j * 32 + 26));
                        Dir->DIR_FileSize = CharIntoDWORD((BYTE *)(buffer + j * 32 + 28));
                        return find_subdir(fat16_ins, Dir, paths, pathDepth, curDepth + 1);
                    }
                    else{
                        strncpy((char *)Dir->DIR_Name, (char *)(buffer + j * 32), 11);
                        Dir->DIR_Attr = buffer[j * 32 + 11];
                        Dir->DIR_NTRes = buffer[j * 32 + 12];
                        Dir->DIR_CrtTimeTenth = buffer[j * 32 + 13];
                        Dir->DIR_CrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 14));
                        Dir->DIR_CrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 16));
                        Dir->DIR_LstAccDate = CharIntoWORD((BYTE *) (buffer + j*32 + 18));
                        Dir->DIR_FstClusHI = CharIntoWORD((BYTE *) (buffer + j * 32 + 20));
                        Dir->DIR_WrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 22));
                        Dir->DIR_WrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 24));
                        Dir->DIR_FstClusLO = CharIntoWORD((BYTE *)(buffer + j * 32 + 26));
                        Dir->DIR_FileSize = CharIntoDWORD((BYTE *)(buffer + j * 32 + 28));
                        return 0;
                    }
                }
                else{ //是文件
                    if(pathDepth != (curDepth + 1))
                        return 1;
                    else{
                        strncpy((char *)Dir->DIR_Name, (char *)(buffer + j * 32), 11);
                        Dir->DIR_Attr = buffer[j * 32 + 11];
                        Dir->DIR_NTRes = buffer[j * 32 + 12];
                        Dir->DIR_CrtTimeTenth = buffer[j * 32 + 13];
                        Dir->DIR_CrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 14));
                        Dir->DIR_CrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 16));
                        Dir->DIR_LstAccDate = CharIntoWORD((BYTE *) (buffer + j*32 + 18));
                        Dir->DIR_FstClusHI = CharIntoWORD((BYTE *) (buffer + j * 32 + 20));
                        Dir->DIR_WrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 22));
                        Dir->DIR_WrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 24));
                        Dir->DIR_FstClusLO = CharIntoWORD((BYTE *)(buffer + j * 32 + 26));
                        Dir->DIR_FileSize = CharIntoDWORD((BYTE *)(buffer + j * 32 + 28));
                        return 0;
                    }
                }
            }
        }
        for(i = 1; i != fat16_ins->Bpb.BPB_SecPerClus; i ++){     //处理簇中的其他扇区
            sector_read(fd, FirstSectorofCluster + i, buffer); //将第i个扇区读入buffer
            for(j = 0; j != 16; j ++){ //处理第i个扇区的16个条目
                if(buffer[j*32] == 0x00)
                    return 1;
                else if(strncpy((char *)(buffer + j*32), paths[curDepth], 11) == 0){ //找到了
                    if((BYTE)buffer[j * 32 + 11] == 0x10){ //目录
                        if(pathDepth != (curDepth + 1)){   //没有到底
                            strncpy((char *)Dir->DIR_Name, (char *)(buffer + j * 32), 11);
                            Dir->DIR_Attr = buffer[j * 32 + 11];
                            Dir->DIR_NTRes = buffer[j * 32 + 12];
                            Dir->DIR_CrtTimeTenth = buffer[j * 32 + 13];
                            Dir->DIR_CrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 14));
                            Dir->DIR_CrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 16));
                            Dir->DIR_LstAccDate = CharIntoWORD((BYTE *) (buffer + j*32 + 18));
                            Dir->DIR_FstClusHI = CharIntoWORD((BYTE *) (buffer + j * 32 + 20));
                            Dir->DIR_WrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 22));
                            Dir->DIR_WrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 24));
                            Dir->DIR_FstClusLO = CharIntoWORD((BYTE *)(buffer + j * 32 + 26));
                            Dir->DIR_FileSize = CharIntoDWORD((BYTE *)(buffer + j * 32 + 28));
                            return find_subdir(fat16_ins, Dir, paths, pathDepth, curDepth + 1);
                        }
                        else{
                            strncpy((char *)Dir->DIR_Name, (char *)(buffer + j * 32), 11);
                            Dir->DIR_Attr = buffer[j * 32 + 11];
                            Dir->DIR_NTRes = buffer[j * 32 + 12];
                            Dir->DIR_CrtTimeTenth = buffer[j * 32 + 13];
                            Dir->DIR_CrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 14));
                            Dir->DIR_CrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 16));
                            Dir->DIR_LstAccDate = CharIntoWORD((BYTE *) (buffer + j*32 + 18));
                            Dir->DIR_FstClusHI = CharIntoWORD((BYTE *) (buffer + j * 32 + 20));
                            Dir->DIR_WrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 22));
                            Dir->DIR_WrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 24));
                            Dir->DIR_FstClusLO = CharIntoWORD((BYTE *)(buffer + j * 32 + 26));
                            Dir->DIR_FileSize = CharIntoDWORD((BYTE *)(buffer + j * 32 + 28));
                            return 0;
                        }
                    }
                    else{ //是文件
                        if(pathDepth != (curDepth + 1))
                            return 1;
                        else{
                            strncpy(Dir->DIR_Name, (char *)(buffer + j * 32), 11);
                            Dir->DIR_Attr = buffer[j * 32 + 11];
                            Dir->DIR_NTRes = buffer[j * 32 + 12];
                            Dir->DIR_CrtTimeTenth = buffer[j * 32 + 13];
                            Dir->DIR_CrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 14));
                            Dir->DIR_CrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 16));
                            Dir->DIR_LstAccDate = CharIntoWORD((BYTE *) (buffer + j*32 + 18));
                            Dir->DIR_FstClusHI = CharIntoWORD((BYTE *) (buffer + j * 32 + 20));
                            Dir->DIR_WrtTime = CharIntoWORD((BYTE *) (buffer + j * 32 + 22));
                            Dir->DIR_WrtDate = CharIntoWORD((BYTE *) (buffer + j * 32 + 24));
                            Dir->DIR_FstClusLO = CharIntoWORD((BYTE *)(buffer + j * 32 + 26));
                            Dir->DIR_FileSize = CharIntoDWORD((BYTE *)(buffer + j * 32 + 28));
                            return 0;
                        }
                    }
                }
            }
        }
    }
    return 1;
}



/**
 * ------------------------------------------------------------------------------
 * FUSE相关的函数实现
**/

void *fat16_init(struct fuse_conn_info *conn)
{
  struct fuse_context *context;             //上下文
  context = fuse_get_context();             //获取上下文信息

  return context->private_data;             //返回私有文件系统数据
}

void fat16_destroy(void *data)              //释放空间
{
  free(data);
}

int fat16_getattr(const char *path, struct stat *stbuf)
{
  FAT16 *fat16_ins;

  struct fuse_context *context;
  context = fuse_get_context();
  fat16_ins = (FAT16 *)context->private_data;            //从数据块中获取文件信息

  memset(stbuf, 0, sizeof(struct stat));                 //一块stat全都置零，应该是用来存储文件信息
  stbuf->st_dev = fat16_ins->Bpb.BS_VollID;              //设备ID
  stbuf->st_blksize = BYTES_PER_SECTOR * fat16_ins->Bpb.BPB_SecPerClus;   //一簇的字节数
  stbuf->st_uid = getuid();                                               //user ID of owner
  stbuf->st_gid = getgid();                                               //group ID of owner

  if (strcmp(path, "/") == 0)                                             //莫得文件
  {
    stbuf->st_mode = S_IFDIR | S_IRWXU;                                   //保护模式
    stbuf->st_size = 0;                                                     //文件大小
    stbuf->st_blocks = 0;                                                   //文件所占块数
    stbuf->st_ctime = stbuf->st_atime = stbuf->st_mtime = 0;                //存取修改时间
  }
  else
  {
    DIR_ENTRY Dir;

    int res = find_root(fat16_ins, &Dir, path);

    if (res == 0)                                                           //如果找到了
    {
      if (Dir.DIR_Attr == ATTR_DIRECTORY)                                   //目录
      {
        stbuf->st_mode = S_IFDIR | 0755; //目录                      
      }
      else
      {
        stbuf->st_mode = S_IFREG | 0755; //一般文件
      }
      stbuf->st_size = Dir.DIR_FileSize; //文件大小

      if (stbuf->st_size % stbuf->st_blksize != 0)
      {
        stbuf->st_blocks = (int)(stbuf->st_size / stbuf->st_blksize) + 1;
      }
      else
      {
        stbuf->st_blocks = (int)(stbuf->st_size / stbuf->st_blksize);
      }

      struct tm t;
      memset((char *)&t, 0, sizeof(struct tm));
      t.tm_sec = Dir.DIR_WrtTime & ((1 << 5) - 1);
      t.tm_min = (Dir.DIR_WrtTime >> 5) & ((1 << 6) - 1);
      t.tm_hour = Dir.DIR_WrtTime >> 11;
      t.tm_mday = (Dir.DIR_WrtDate & ((1 << 5) - 1));
      t.tm_mon = (Dir.DIR_WrtDate >> 5) & ((1 << 4) - 1);
      t.tm_year = 80 + (Dir.DIR_WrtDate >> 9);
      stbuf->st_ctime = stbuf->st_atime = stbuf->st_mtime = mktime(&t);
    }
  }
  return 0;
}

int fat16_readdir(const char *path, void *buffer, fuse_fill_dir_t filler,
                  off_t offset, struct fuse_file_info *fi)
{
  FAT16 *fat16_ins;
  BYTE sector_buffer[BYTES_PER_SECTOR];
  int i,j;  /* 用于统计已读取的扇区数 */
  BYTE temp[12];
  memset((void *)temp, 0, 12);
  struct fuse_context *context;
  context = fuse_get_context();
  fat16_ins = (FAT16 *)context->private_data;
  
  if (strcmp(path, "/") == 0)    //需要显示的是根目录
  {
    DIR_ENTRY Root;

    /** TODO: TOTEST
     * 将root directory下的文件或目录通过filler填充到buffer中
     * 注意不需要遍历子目录
    **/
    for (i = 0; i != fat16_ins->Bpb.BPB_RootEntCnt/16; i++)           //遍历32扇区
    {
        sector_read(fat16_ins->fd, fat16_ins->FirstRootDirSecNum + i, sector_buffer);
        for(j = 0; j != fat16_ins->Bpb.BPB_BytsPerSec/32; j ++){    //遍历一个扇区的条目
            if(sector_buffer[32*j] != 0x00 && sector_buffer[32*j + 11] != 0x0f && sector_buffer[32*j + 8] != 0x00 && sector_buffer[32*j] != 0xe5){
                strncpy(Root.DIR_Name, (char *)(sector_buffer + 32*j), 11);
                const char *filename = (const char *)path_decode(Root.DIR_Name);
				printf("This is in Root dictionary(%d, %d)\n", i , j);
				printf("-----------------------------");
				printf("The filename is %s \n", filename);
                filler(buffer, filename, NULL, 0);
            }   
        }
    }
  }
  else                    //需要显示的是子目录
  {
    DIR_ENTRY Dir;
 
    /** TODO:
     * 通过find_root获取path对应的目录的目录项，
     * 然后访问该目录，将其下的文件或目录通过filler填充到buffer中，
     * 同样注意不需要遍历子目录
     * Hint: 需要考虑目录大小，可能跨扇区，跨簇
    **/
    find_root(fat16_ins, &Dir, path);
    WORD ClusterN, FatClusEntryVal, FirstSectorofCluster;
    ClusterN = Dir.DIR_FstClusLO; //子目录开始的首簇号
    while(ClusterN != 0xffff){    //知道簇号为0xffff
        first_sector_by_cluster(fat16_ins, ClusterN, &FatClusEntryVal, &FirstSectorofCluster, sector_buffer); //读第一个扇区
        ClusterN = FatClusEntryVal;
        for(j = 0; j != fat16_ins->Bpb.BPB_BytsPerSec/32; j ++){   //处理该簇第一个扇区
            if(sector_buffer[j*32] == 0x00)
                return 0;
			if(sector_buffer[32*j + 11] != 0x0f && sector_buffer[32*j + 8] != 0x00 && sector_buffer[32*j] != 0xe5){
            	strncpy(temp, (char *)(sector_buffer + 32*j), 11);
            	const char *filename = (const char *)path_decode(temp);
	    		printf("This is in %s \n", path);
				printf("-----------------------------");
				printf("The filename is %s \n", filename);
            	filler(buffer, filename, NULL, 0);
			}
        }
        for(i = 1; i != fat16_ins->Bpb.BPB_SecPerClus; i ++){
            sector_read(fat16_ins->fd, FirstSectorofCluster + i ,sector_buffer);
            for(j = 0; j != fat16_ins->Bpb.BPB_BytsPerSec/32; j ++){   //处理该簇的其他扇区
                if(sector_buffer[j*32] == 0x00)
                    return 0;
				if(sector_buffer[32*j + 11] != 0x0f && sector_buffer[32*j + 8] != 0x00 && sector_buffer[32*j] != 0xe5){
                	strncpy(temp, (char *)(sector_buffer + 32*j), 11);
                	const char *filename = (const char *)path_decode(temp);
					printf("This is in %s \n", path);
					printf("-----------------------------");
					printf("The filename is %s \n", filename);
                	filler(buffer, filename, NULL, 0);
				}
            }
        }
    }
  }
  return 0;
}

/** TODO:
 * 从path对应的文件的offset字节处开始读取size字节的数据到buffer中，并返回实际读取的字节数
 * 
 * Hint: 文件大小属性是Dir.DIR_FileSize；当offset超过文件大小时，应该返回0
**/
int fat16_read(const char *path, char *buffer, size_t size, off_t offset,
               struct fuse_file_info *fi)
{
  FAT16 *fat16_ins;
  size_t size_real;
  size_t end;
  struct fuse_context *context;
  context = fuse_get_context();
  fat16_ins = (FAT16 *)context->private_data;
  FILE *fd = fat16_ins->fd;
  DIR_ENTRY Dir;
  if((find_root(fat16_ins, &Dir, path) == 1) || offset > Dir.DIR_FileSize){    //没找到或者offset超过FileSize
    return 0;
  }
  else{
    if((offset + size) > Dir.DIR_FileSize)
        size_real = Dir.DIR_FileSize - offset;
    else
        size_real = size;
    end = size_real + offset;
    off_t BytePerClus = (fat16_ins->Bpb.BPB_BytsPerSec * fat16_ins->Bpb.BPB_SecPerClus);
    off_t startCluster = offset / BytePerClus;           //文件
    off_t startOffset = offset % BytePerClus;
    off_t endCluster = (off_t)end / BytePerClus;
    off_t endOffset = (off_t)end % BytePerClus;
    off_t ClusterNum = 0;           //读簇数目计数器
    off_t ByteCount = 0;            //buffer偏移计数器
    WORD ClusterN = Dir.DIR_FstClusLO;
    while(ClusterN != 0xffff){
        if(startCluster <= ClusterNum && ClusterNum <= endCluster){
            if(startCluster == ClusterNum){             //在开始簇
                fseek(fd, (long)((ClusterN-2) * BytePerClus + fat16_ins->FirstDataSector * fat16_ins->Bpb.BPB_BytsPerSec + startOffset), SEEK_SET);
                fread(buffer + ByteCount, (size_t)(BytePerClus - startOffset), 1 ,fd);
                ByteCount = ByteCount + BytePerClus - startOffset;
            }
            else if(endCluster == ClusterNum){          //在结束簇
                fseek(fd, (long)((ClusterN-2) * BytePerClus + fat16_ins->FirstDataSector * fat16_ins->Bpb.BPB_BytsPerSec), SEEK_SET);
                fread(buffer + ByteCount, (size_t)(endOffset), 1 ,fd);   
                ByteCount = ByteCount + endOffset;     
            }
            else{                                       //在正常簇
                fseek(fd, (long)((ClusterN-2) * BytePerClus + fat16_ins->FirstDataSector * fat16_ins->Bpb.BPB_BytsPerSec), SEEK_SET);
                fread(buffer + ByteCount, (size_t)(BytePerClus), 1 ,fd);
                ByteCount = ByteCount + BytePerClus;
            }
        }
        else if(ClusterNum > endCluster){
            break;
        }
        ClusterN = fat_entry_by_cluster(fat16_ins, ClusterN);
        ClusterNum = ClusterNum + 1;
    }
    return size_real;
  }
  return 0;
}



/**
 * ------------------------------------------------------------------------------
 * 测试函数
**/

void test_path_split() {
  printf("#1 running %s\n", __FUNCTION__);

  char s[][32] = {"/texts", "/dir1/dir2/file.txt", "/.Trash-100"};
  int dr[] = {1, 3, 1};
  char sr[][3][32] = {{"TEXTS      "}, {"DIR1       ", "DIR2       ", "FILE    TXT"}, {"        TRA"}};
  int i, j, r;
  for (i = 0; i < sizeof(dr) / sizeof(dr[0]); i++) {
  
    char **ss = path_split(s[i], &r);
    assert(r == dr[i]);
    for (j = 0; j < dr[i]; j++) {
      assert(strcmp(sr[i][j], ss[j]) == 0);
      free(ss[j]);
    }
    free(ss);
    printf("test case %d: OK\n", i + 1);
  }

  printf("success in %s\n\n", __FUNCTION__);
}

void test_path_decode() {
  printf("#2 running %s\n", __FUNCTION__);

  char s[][32] = {"..        ", "FILE    TXT", "ABCD    RM "};
  char sr[][32] = {"..", "file.txt", "abcd.rm"};

  int i, j, r;
  for (i = 0; i < sizeof(s) / sizeof(s[0]); i++) {
    char *ss = (char *) path_decode(s[i]);
    assert(strcmp(ss, sr[i]) == 0);
    free(ss);
    printf("test case %d: OK\n", i + 1);
  }

  printf("success in %s\n\n", __FUNCTION__);
}

void test_pre_init_fat16() {
  printf("#3 running %s\n", __FUNCTION__);

  FAT16 *fat16_ins = pre_init_fat16();

  assert(fat16_ins->FirstRootDirSecNum == 124);
  assert(fat16_ins->FirstDataSector == 156);
  assert(fat16_ins->Bpb.BPB_RsvdSecCnt == 4);
  assert(fat16_ins->Bpb.BPB_RootEntCnt == 512);
  assert(fat16_ins->Bpb.BS_BootSig == 41);
  assert(fat16_ins->Bpb.BS_VollID == 1576933109);
  assert(fat16_ins->Bpb.Signature_word == 43605);
  
  fclose(fat16_ins->fd);
  free(fat16_ins);
  
  printf("success in %s\n\n", __FUNCTION__);
}

void test_fat_entry_by_cluster() {
  printf("#4 running %s\n", __FUNCTION__);

  FAT16 *fat16_ins = pre_init_fat16();

  int cn[] = {1, 2, 4};
  int ce[] = {65535, 0, 65535};

  int i;
  for (i = 0; i < sizeof(cn) / sizeof(cn[0]); i++) {
    int r = fat_entry_by_cluster(fat16_ins, cn[i]);
    assert(r == ce[i]);
    printf("test case %d: OK\n", i + 1);
  }
  
  fclose(fat16_ins->fd);
  free(fat16_ins);

  printf("success in %s\n\n", __FUNCTION__);
}

void test_find_root() {
  printf("#5 running %s\n", __FUNCTION__);

  FAT16 *fat16_ins = pre_init_fat16();

  char path[][32] = {"/dir1", "/makefile", "/log.c"};
  char names[][32] = {"DIR1       ", "MAKEFILE   ", "LOG     C  "};
  int others[][3] = {{100, 4, 0}, {100, 8, 226}, {100, 3, 517}};

  int i;
  for (i = 0; i < sizeof(path) / sizeof(path[0]); i++) {
    DIR_ENTRY Dir;
    find_root(fat16_ins, &Dir, path[i]);
    assert(strncmp(Dir.DIR_Name, names[i], 11) == 0);
    assert(Dir.DIR_CrtTimeTenth == others[i][0]);
    assert(Dir.DIR_FstClusLO == others[i][1]);
    assert(Dir.DIR_FileSize == others[i][2]);

    printf("test case %d: OK\n", i + 1);
  }
  
  fclose(fat16_ins->fd);
  free(fat16_ins);

  printf("success in %s\n\n", __FUNCTION__);
}

void test_find_subdir() {
  printf("#6 running %s\n", __FUNCTION__);

  FAT16 *fat16_ins = pre_init_fat16();

  char path[][32] = {"/dir1/dir2", "/dir1/dir2/dir3", "/dir1/dir2/dir3/test.c"};
  char names[][32] = {"DIR2       ", "DIR3       ", "TEST    C  "};
  int others[][3] = {{100, 5, 0}, {0, 6, 0}, {0, 7, 517}};

  int i;
  for (i = 0; i < sizeof(path) / sizeof(path[0]); i++) {
    DIR_ENTRY Dir;
    find_root(fat16_ins, &Dir, path[i]);
    assert(strncmp(Dir.DIR_Name, names[i], 11) == 0);
    assert(Dir.DIR_CrtTimeTenth == others[i][0]);
    assert(Dir.DIR_FstClusLO == others[i][1]);
    assert(Dir.DIR_FileSize == others[i][2]);

    printf("test case %d: OK\n", i + 1);
  }
  
  fclose(fat16_ins->fd);
  free(fat16_ins);

  printf("success in %s\n\n", __FUNCTION__);
}


struct fuse_operations fat16_oper = {
    .init = fat16_init,
    .destroy = fat16_destroy,
    .getattr = fat16_getattr,
    .readdir = fat16_readdir,
    .read = fat16_read
    };

int main(int argc, char *argv[])
{
  int ret;

  if (strcmp(argv[1], "--test") == 0) {
    printf("--------------\nrunning test\n--------------\n");
    FAT_FILE_NAME = "fat16_test.img";
    test_path_split();
    test_path_decode();
    test_pre_init_fat16();
    test_fat_entry_by_cluster();
    test_find_root();
    test_find_subdir();
    exit(EXIT_SUCCESS);
  }

  FAT16 *fat16_ins = pre_init_fat16();

  ret = fuse_main(argc, argv, &fat16_oper, fat16_ins);

  return ret;
}
