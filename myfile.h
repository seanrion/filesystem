#ifndef __MYFILE_H
#define __MYFILE_H

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <semaphore.h>

#define NUM 121
#define PATHLENGTH 200
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
#define MAGICNUM 0xdec0de
#define BLOCKNUM 4096
#define INODENUM 1024
#define INODEBASE 1
#define DATABLOCKBASE 33
typedef struct super_block {
    int32_t magic_num;                  // 幻数
    int32_t free_block_count;           // 空闲数据块数
    int32_t free_inode_count;           // 空闲inode数
    int32_t dir_inode_count;            // 目录inode数
    uint32_t block_map[128];            // 数据块占用位图
    uint32_t inode_map[32];             // inode占用位图
} sp_block;

#define DIR 0
#define FILE 1
#define DIR_MAX_SIZE 6114
struct inode {
    uint32_t size;              // 文件大小
    uint16_t file_type;         // 文件类型（文件/文件夹）
    uint16_t link;              // 连接数
    uint32_t block_point[6];    // 数据块指针
};//32字节

struct dir_item {               // 目录项一个更常见的叫法是 dirent(directory entry)
    uint32_t inode_id;          // 当前目录项表示的文件/目录的对应inode
    uint16_t valid;             // 当前目录项是否有效 
    uint8_t type;               // 当前目录项类型（文件/目录）
    char name[121];             // 目录项表示的文件/目录的文件名/目录名
};//128字节

int disk_read_1K_block(uint32_t block_num, int8_t *buf);
int disk_write_1K_block(uint32_t block_num, int8_t *buf);
int initSystem();
int openSystem();
int closeSystem();
void exitSystem(); 
void init_sp_block();
int write_sp_block();
int read_sp_block();
int initdir(struct inode *dir, uint32_t parent_inode_id, uint32_t self_inode_id);
void initRootDir();
char* getPath();
void showDir();
int findItemInDir(struct inode myDir, char unitName[]);
int32_t blockid_of_inode(int32_t inode_id);
int32_t offset_of_inode(int32_t inode_id);
int changeDir(char dirName[]);
int find_unused_block();
void use_block(int32_t block_id);
int find_unused_inode();
void use_inode(int32_t inode_id);
int addDirUnit(struct inode *myDir, char fileName[], int type);
int creatDir(struct inode *myDir,char dirName[]);
int creatFile(struct inode *myDir,char fileName[]);
int copyFile(char dst_path[], char src_path[]);
char *get_inode_from_path(struct inode *src_inode, char src_path[]);
char *get_dirinode_from_path(struct inode *src_inode, char src_path[]);
char *get_fileinode_from_path(struct inode *src_inode, char src_path[]);
#endif