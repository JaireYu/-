#include"fat16.h"
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/* Unit size */
#define BYTES_PER_SECTOR 512
#define BYTES_PER_DIR 32

/* File property */
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

#define MAX_SHORT_NAME_LEN 13

typedef uint8_t BYTE;
typedef uint16_t WORD;
typedef uint32_t DWORD;

typedef struct {
    BYTE BS_jmpBoot[3];   //跳转命令
    BYTE BS_OEMName[8];   //OEM NAME
    WORD BPB_BytsPerSec;  //每扇区512k字节
    BYTE BPB_SecPerClus;  //每簇32扇区 32*512k = 65536(2^16)
    WORD BPB_RsvdSecCnt;  //保留扇区数
    BYTE BPB_NumFATS;     //FAT数
    WORD BPB_RootEntCnt;  //Root Entery count  11
    WORD BPB_TotSec16;    //FAT占的扇区数       13
    BYTE BPB_Media;       //介质描述             15
    WORD BPB_FATSz16;     //每个FAT的扇区数     16
    WORD BPB_SecPerTrk;   //每个磁道的扇区数    18
    WORD BPB_NumHeads;    //磁头数              1a
    DWORD BPB_HiddSec;    //隐藏扇区数          1c
    DWORD BPB_TotSec32;   //NTFS是否使用        20
    BYTE BS_DrvNum;       //中断驱动器号            24
    BYTE BS_Reserved1;    //未使用              26
    BYTE BS_BootSig;      //扩展引导标记              28
    DWORD BS_VollID;      //卷序列号             28
    BYTE BS_VollLab[11];  //卷标签               2a
    BYTE BS_FilSysType[8];//FATname             36
    BYTE Reserved2[448];  //引导程序执行代码      3E
    WORD Signature_word;  //扇区结束标志         1FE
} __attribute__ ((packed)) BPB_BS;

/* FAT16 volume data with a file handler of the FAT16 image file */
typedef struct
{
    FILE *fd;    //file descriptor table
    DWORD FirstRootDirSecNum;
    DWORD FirstDataSector;
    BPB_BS Bpb;
} FAT16;
void test_pre_init_fat16() {
    printf("#3 running %s\n", __FUNCTION__);

    FAT16 *fat16_ins = pre_init_fat16();

    assert(fat16_ins->FirstRootDirSecNum == 124);   //RootDir始于124扇区
    assert(fat16_ins->FirstDataSector == 156);      //RootDir终于156扇区亦即数据区开始的扇区
    assert(fat16_ins->Bpb.BPB_RsvdSecCnt == 4);
    assert(fat16_ins->Bpb.BPB_RootEntCnt == 512);
    assert(fat16_ins->Bpb.BS_BootSig == 41);
    assert(fat16_ins->Bpb.BS_VollID == 1576933109);
    assert(fat16_ins->Bpb.Signature_word == 43605);

    fclose(fat16_ins->fd);
    free(fat16_ins);

    printf("success in %s\n\n", __FUNCTION__);
}
DWORD get_DWORD(FILE *fp){
    BYTE *s;
    DWORD i;
    s = (BYTE *)&i;
    s[0]= (BYTE)fgetc(fp);
    s[1]= (BYTE)fgetc(fp);
    s[2]= (BYTE)fgetc(fp);
    s[3]= (BYTE)fgetc(fp);
    return i;
}

WORD get_WORD(FILE *fp){
    BYTE *s;
    WORD i;
    s = (BYTE *)&i;
    s[0]= (BYTE)fgetc(fp);
    s[1]= (BYTE)fgetc(fp);
    return i;
}

FAT16 *pre_init_fat16(void)
{
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

/** TODO:
   * 初始化fat16_ins的其余成员变量
   * Hint: root directory的大小与Bpb.BPB_RootEntCnt有关，并且是扇区对齐的
**/
    fgets((char *)fat16_ins->Bpb.BS_jmpBoot, 3, fd);
    fgets((char *)fat16_ins->Bpb.BS_OEMName, 8, fd);
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
    fgets((char *)fat16_ins->Bpb.BS_VollLab, 11, fd);
    fgets((char *)fat16_ins->Bpb.BS_FilSysType, 8, fd);
    fgets((char *)fat16_ins->Bpb.Reserved2, 448, fd);
    fat16_ins->Bpb.Signature_word = get_WORD(fd);
    fat16_ins->FirstRootDirSecNum = fat16_ins->Bpb.BPB_NumFATS * fat16_ins->Bpb.BPB_FATSz16 + fat16_ins->Bpb.RsvdSecCnt;
    fat16_ins->FirstDataSector = fat16_ins->Bpb.BPB_RootEntCnt;
    return fat16_ins;
}