#include "myfile.h"
#include "disk.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

struct inode rootDirinode; //根目录
struct inode currentDirinode;  //当前位置
int8_t path[PATHLENGTH];       //保存当前绝对路径

static int8_t spbblock[1024];//申请1KB空间做超级块空间
static sp_block* spb;

static int8_t block_buffer[1024];//申请1KB空间做磁盘数据块缓存
static int8_t inode_buffer[1024];//申请1KB空间做磁盘inode块缓存
static int8_t *block_buf;
static int8_t *inode_buf;

int disk_read_1K_block(uint32_t block_num, int8_t *buf){
    if (-1 == disk_read_block(block_num*2, buf)){
        printf("fail to read diskblock %d.\n", block_num * 2);
        return -1;
    }
    if (-1 == disk_read_block(block_num * 2 + 1, buf + DEVICE_BLOCK_SIZE))
    {
        printf("fail to read diskblock %d.\n", block_num * 2 + 1);
        return -1;
    }
    return 0;
}
int disk_write_1K_block(uint32_t block_num, int8_t *buf){
    if (-1 == disk_write_block(block_num*2, buf)){
        printf("fail to write diskblock %d.\n", block_num * 2);
        return -1;
    }
    if (-1 == disk_write_block(block_num * 2 + 1, buf + DEVICE_BLOCK_SIZE))
    {
        printf("fail to write diskblock %d.\n", block_num * 2 + 1);
        return -1;
    }
    return 0;
}

int initSystem(){
    spb = (sp_block *)spbblock;
    block_buf = block_buffer;
    inode_buf = inode_buffer;
    init_sp_block();
    initRootDir();

    if (-1==write_sp_block()) {
        printf("Initial super bolck failed.\n");
        return -1;
    }
    return 0;
}

int openSystem() {
    spb = (sp_block *)spbblock;
    block_buf = block_buffer;
    inode_buf = inode_buffer;
    if (open_disk() == -1)
    {
        printf("Open disk error.\n");
        return -1;
    }
    if ((read_sp_block()!=-1) && spb->magic_num == MAGICNUM) {
        return 0;
    } else {
        printf("File System Unkonwn or didn't exist. Initial a file system?(y/n)\n");
        int8_t option;
        scanf("%c", &option);
        if (option == 'y') {
            return initSystem();
        } else {
            exitSystem();
        }
    }
}

int closeSystem() {
    if (-1==write_sp_block()) {
        printf("Save super bolck failed.\n");
        return -1;
    }
    if (close_disk() == -1) {
        printf("Close disk failed.\n");
    }
    return 0;

}

void exitSystem(){
    closeSystem();
    printf("File System Exit.\n");
    exit(0);
}

void init_sp_block() {
    memset(spb,0,DEVICE_BLOCK_SIZE*2);
    spb->magic_num = MAGICNUM;
    spb->free_block_count = BLOCKNUM;
    spb->free_inode_count = INODENUM;
    //标注第0到第32块数据块被超级快和inode块占用了
    spb->block_map[0] = 0xffffffff;
    spb->block_map[1] = 1;
}

int write_sp_block() {
    if(spb==NULL)
        return -1;
    return disk_write_1K_block(0, (int8_t *)spb);
}

int read_sp_block() {
    if(spb==NULL)
        return -1;
    return disk_read_1K_block(0, (int8_t *)spb);
}

//初始化目录,添加到自身的目录项和到父目录的目录项
int initdir(struct inode* dir,uint32_t parent_inode_id, uint32_t self_inode_id)
{
    int block_id = find_unused_block();
    if (block_id == -1)
        return -1;
    spb->dir_inode_count++;
    use_block(block_id);
    dir->block_point[0] = block_id;
    if (disk_read_1K_block(block_id, block_buf) == -1)
        return -1;

    struct dir_item *new_dir_item = (struct dir_item *)block_buf;
    new_dir_item->inode_id = self_inode_id;
    new_dir_item->type = DIR;
    new_dir_item->valid = 1;
    int8_t self[] = ".";
    strcpy(new_dir_item->name, self);

    new_dir_item++;
    new_dir_item->inode_id = parent_inode_id;
    new_dir_item->type = DIR;
    new_dir_item->valid = 1;
    int8_t parent[] = "..";
    strcpy(new_dir_item->name, parent);

    if (disk_write_1K_block(block_id, block_buf) == -1)
        return -1;

    dir->size = 2*sizeof(struct dir_item);
    dir->block_point[0] = block_id;
    return 0;
}

//初始化根目录
void initRootDir()
{
    if (disk_read_1K_block(INODEBASE, inode_buf) == -1)
        return;
    rootDirinode = *(struct inode *)inode_buf;
    if(rootDirinode.link == 1){
        currentDirinode = rootDirinode;
        path[0] = '/';
        path[1]='\0';
        return;
    }
    rootDirinode.file_type = DIR;
    rootDirinode.size = 0;
    rootDirinode.link = 1;
    spb->inode_map[0] = 1;

    initdir(&rootDirinode,0,0);
    currentDirinode = rootDirinode;
    *(struct inode *)inode_buf = rootDirinode;
    if (disk_write_1K_block(INODEBASE, inode_buf) == -1)
        return;
    //初始化初始绝对路径
    path[0] = '/';
    path[1]='\0';
}


//获得绝对路径
char* getPath()
{
    return path;
}


//展示当前目录 ls
void showDir()
{
    int size = 0;
    for(int i=0; i<6; i++)
    {
        if(currentDirinode.block_point[i]==0){
            return;
        }

        if(disk_read_1K_block(currentDirinode.block_point[i],block_buf)==-1)
            return;

        struct dir_item *dir = (struct dir_item *)block_buf;
        for(int j=0;j<1024;j+=sizeof(struct dir_item)){
            if(dir->valid==0){
                dir++;
                continue;
            }
            if(dir->type == DIR){
                printf("%s\tdirectory\n",dir->name);
            }
            if(dir->type == FILE){
                printf("%s\tfile\n",dir->name);
            }
            dir++;
            size += sizeof(struct dir_item);
            if(size>=currentDirinode.size)
                return;
        }
    }
    return;
}

//从目录中查找目录项目
int findItemInDir(struct inode myDir, char unitName[])
{
    int size = 0;
    for(int i=0; i<6; i++)
    {
        if(myDir.block_point[i]==0)
            return -1;

        if (disk_read_1K_block(myDir.block_point[i], block_buf) == -1)
            return -1;

        struct dir_item *dir = (struct dir_item *)block_buf;
        for(int j=0;j<1024;j+=sizeof(struct dir_item)){
            if(dir->valid==0){
                dir++;
                continue;
            }
            if (strcmp(unitName, dir->name) == 0){
                return dir->inode_id;
            }
            dir++;
            size += sizeof(struct dir_item);
            if (size >= myDir.size)
                return -1;
        }
    }
    return -1;
}

int32_t blockid_of_inode(int32_t inode_id)
{
    return inode_id / (1024 / sizeof(struct inode)) + INODEBASE;
}
int32_t offset_of_inode(int32_t inode_id)
{
    return inode_id % (1024 / sizeof(struct inode));
}

//切换目录 cd
int changeDir(char dirName[])
{
    //目录项在目录位置
    int32_t inode_id = findItemInDir(currentDirinode, dirName);
    //不存在
    if (inode_id == -1){
        printf("%s file not found\n",dirName);
        return -1;
    }

    int32_t block_id = blockid_of_inode(inode_id);
    int32_t inode_offset = offset_of_inode(inode_id);
    if (disk_read_1K_block(block_id, inode_buf) == -1)
        return -1;
    struct inode* next_inode = (struct inode*)inode_buf;
    next_inode += inode_offset;
    if (next_inode->file_type != DIR){
        printf("%s not a dir\n",dirName);
        return -1;
    }
    //修改当前目录
    currentDirinode = *next_inode;
    //修改全局绝对路径
    if(strcmp(dirName, ".") == 0){
        return 0;
    }else if(strcmp(dirName, "..") == 0)
    {
        //回退绝对路径
        int len = strlen(path);
        for(int i=len-2;i>=0;i--)
            if(path[i] == '/')
            {
                path[i+1]='\0';
                break;
            }
    }else {
        strcat(path, dirName);
        strcat(path, "/");
    }

    return 0;
}

//寻找空闲的数据块
int find_unused_block(){
    if(spb->free_block_count==0){
        printf("disk block is full, try to delete some file\n");
        return -1;
    }
    for (int i = 0; i < 128; i++)
    {
        if (spb->block_map[i] != 0xffffffff)
        {
            for (int block = 0; block < 32; block++)
            {
                if ((spb->block_map[i] & (1 << block)) == 0){
                    return i*32 + block;
                }
            }
        }
    }
    printf("disk block is full, try to delete some file\n");
    return -1;
}

//占用该数据块
void use_block(int32_t block_id)
{
    int i = block_id / 32;
    block_id = block_id % 32;
    spb->block_map[i] |= (1 << block_id);
    spb->free_block_count--;
    if (-1 == write_sp_block()){
        printf("Save super bolck failed.\n");
        return ;
    }
}

//寻找空闲的inode块
int find_unused_inode(){
    if(spb->free_inode_count==0){
        printf("disk inode is full, try to delete some file\n");
        return -1;
    }
    for (int i = 0; i < 32; i++)
    {
        if (spb->inode_map[i] != 0xffffffff)
        {
            for (int inode = 0; inode < 32; inode++)
            {
                if ((spb->inode_map[i] & (1 << inode)) == 0)
                {
                    return i * 32 + inode;
                }
            }
        }
    }
    printf("disk inode is full, try to delete some file\n");
    return -1;
}

//占用该inode块
void use_inode(int32_t inode_id)
{
    int i = inode_id / 32;
    inode_id = inode_id % 32;
    spb->inode_map[i] |= (1 << inode_id);
    spb->free_inode_count--;
    if (-1 == write_sp_block()){
        printf("Save super bolck failed.\n");
        return ;
    }
}

//添加目录项，使用栈中变量作myDir，返回目录项inode_id，调用完后将myDir写回磁盘
int addDirUnit(struct inode *myDir, char fileName[], int type)
{
    //检测目录表是否已满
    if (myDir->size == DIR_MAX_SIZE)
    {
        printf("dir is full, try to delete some file\n");
        return -1;
    }

    //是否存在同名文件
    if (findItemInDir(*myDir, fileName) != -1)
    {
        printf("file already exist\n");
        return -1;
    }

    int i;
    for (i = 0; i < 6; i++)
    {
        if(myDir->block_point[i]==0)
            break;

        if (disk_read_1K_block(myDir->block_point[i], block_buf) == -1)
            return -1;

        struct dir_item *item = (struct dir_item *)block_buf;
        for (int j = 0; j < 1024; j += sizeof(struct dir_item))
        {
            if (item->valid == 1)
            {
                item++;
                continue;
            }
            int inode_id = find_unused_inode();
            if (inode_id == -1)
                return -1;
            item->valid = 1;
            item->type = type;
            strcpy(item->name, fileName);
            item->inode_id = inode_id;
            if (disk_write_1K_block(myDir->block_point[i], block_buf) == -1)
                return -1;
            use_inode(inode_id);

            int32_t block_id = blockid_of_inode(inode_id);
            int32_t inode_offset = offset_of_inode(inode_id);
            if (disk_read_1K_block(block_id, inode_buf) == -1)
                return -1;
            struct inode *new_inode = (struct inode *)inode_buf;
            new_inode += inode_offset;
            new_inode->size = 0;
            new_inode->link = 1;
            new_inode->file_type = type;
            new_inode->block_point[0] = 0;
            if (disk_write_1K_block(block_id, inode_buf) == -1)
                return -1;
            myDir->size += sizeof(struct dir_item);
            return inode_id;
        }
    }
    if(i==6){
        printf("disk block is full, try to delete some file\n");
        return -1;
    }

    //使用新的block块和inode块
    int block_id = find_unused_block();
    if (block_id == -1)
        return -1;
    int inode_id = find_unused_inode();
    if (inode_id == -1)
        return -1;
    if (disk_read_1K_block(block_id, block_buf) == -1)
        return -1;
    struct dir_item *new_item = (struct dir_item *)block_buf;
    new_item->inode_id = inode_id;
    new_item->type = type;
    new_item->valid = 1;
    strcpy(new_item->name, fileName);

    int32_t inode_block_id = blockid_of_inode(inode_id);
    int32_t inode_offset = offset_of_inode(inode_id);
    if (disk_read_1K_block(inode_block_id, inode_buf) == -1)
        return -1;
    struct inode *new_inode = (struct inode *)inode_buf;
    new_inode->size = 0;
    new_inode->link = 1;
    new_inode->file_type = type;
    new_inode->block_point[0] = 0;

    if (disk_write_1K_block(block_id, block_buf) == -1)
        return -1;
    if (disk_write_1K_block(inode_block_id, inode_buf) == -1)
        return -1;
    use_block(block_id);
    use_inode(inode_id);

    //修改父目录的inode的部分内容
    myDir->block_point[i] = block_id;
    myDir->size += sizeof(struct dir_item);

    return inode_id;
}

//创建目录 mkdir
int creatDir(struct inode *myDir,char dirName[])
{
    if (strlen(dirName) >= NUM)
    {
        printf("file name too long\n");
        return -1;
    }

    //将目录作为目录项添加到当前目录
    uint32_t child_inode_id = addDirUnit(myDir, dirName, DIR);
    if (child_inode_id == -1)
        return -1;

    //将当前目录的变更写回磁盘
    //先找到当前目录的inode_id
    if (disk_read_1K_block(myDir->block_point[0], block_buf) == -1)
        return -1;
    struct dir_item *item = (struct dir_item *)block_buf;
    uint32_t parent_inode_id = item->inode_id;
    int32_t inode_block_id = blockid_of_inode(parent_inode_id);
    int32_t inode_offset = offset_of_inode(parent_inode_id);
    if (disk_read_1K_block(inode_block_id, inode_buf) == -1)
        return -1;
    struct inode *old_inode = (struct inode *)inode_buf;
    old_inode += inode_offset;
    *old_inode = *myDir;
    if (disk_write_1K_block(inode_block_id, inode_buf) == -1)
        return -1;

    //最后将新建目录初始化，写回磁盘
    inode_block_id = blockid_of_inode(child_inode_id);
    inode_offset = offset_of_inode(child_inode_id);
    if (disk_read_1K_block(inode_block_id, inode_buf) == -1)
        return -1;
    struct inode *child_inode = (struct inode *)inode_buf;
    child_inode += inode_offset;
    initdir(child_inode, parent_inode_id, child_inode_id);
    if (disk_write_1K_block(inode_block_id, inode_buf) == -1)
        return -1;

    return 0;
}

//创建文件 touch
int creatFile(struct inode *myDir,char fileName[])
{
    //检测文件名字长度
    if(strlen(fileName) >= NUM)
    {
        printf("file name too long\n");
        return -1;
    }

    //添加到目录项
    uint32_t child_inode_id = addDirUnit(myDir, fileName, FILE);
    if (child_inode_id == -1)
        return -1;

    //将当前目录的变更写回磁盘
    //先找到当前目录的inode_id
    if (disk_read_1K_block(myDir->block_point[0], block_buf) == -1)
        return -1;
    struct dir_item *item = (struct dir_item *)block_buf;
    uint32_t parent_inode_id = item->inode_id;
    int32_t inode_block_id = blockid_of_inode(parent_inode_id);
    int32_t inode_offset = offset_of_inode(parent_inode_id);
    if (disk_read_1K_block(inode_block_id, inode_buf) == -1)
        return -1;
    struct inode *old_inode = (struct inode *)inode_buf;
    old_inode += inode_offset;
    *old_inode = *myDir;
    if (disk_write_1K_block(inode_block_id, inode_buf) == -1)
        return -1;

    return 0;
}

char *get_inode_from_path(struct inode *src_inode, char src_path[])
{
    char *dirName;
    char *Name;
    dirName = strtok(src_path, "/");
    while (dirName)
    {
        Name = dirName;
        if (src_inode->file_type != DIR)
        {
            printf("%s not a dir\n", dirName);
            return NULL;
        }
        int32_t inode_id = findItemInDir(*src_inode, dirName);
        if (inode_id == -1)
        {
            printf("%s file not found\n", dirName);
            return NULL;
        }

        int32_t block_id = blockid_of_inode(inode_id);
        int32_t inode_offset = offset_of_inode(inode_id);
        if (disk_read_1K_block(block_id, inode_buf) == -1)
            return NULL;
        struct inode *next_inode = (struct inode *)inode_buf;
        next_inode += inode_offset;
        *src_inode = *next_inode;
        dirName = strtok(NULL, "/");
    }

    return Name;
}

char* get_dirinode_from_path(struct inode *src_inode, char src_path[])
{
    char *dir;
    dir = get_inode_from_path(src_inode, src_path);
    if (dir == NULL)
        return NULL;
    if (src_inode->file_type != DIR)
    {
        printf("%s not a dir\n", dir);
        return NULL;
    }
    return dir;
}

char* get_fileinode_from_path(struct inode *src_inode, char src_path[])
{
    char *file;
    file = get_inode_from_path(src_inode, src_path);
    if (file == NULL)
        return NULL;
    if (src_inode->file_type != FILE)
    {
        printf("%s not a file\n", file);
        return NULL;
    }
    return file;
}

int copyFile(char dst_path[], char src_path[])
{
    if (strlen(dst_path) >= NUM || strlen(src_path) >= NUM)
    {
        printf("path too long\n");
        return -1;
    }
    struct inode src_inode = currentDirinode;
    struct inode dst_inode = currentDirinode;
    char *src_fileName;
    char *dst_dirName;
    src_fileName = get_fileinode_from_path(&src_inode, src_path);
    dst_dirName = get_dirinode_from_path(&dst_inode, dst_path);
    if (src_fileName == NULL)
        return -1;
    if (dst_dirName == NULL)
        return -1;

    creatFile(&dst_inode, src_fileName);

    //将数据块链接上
    int32_t inode_id = findItemInDir(dst_inode, src_fileName);
    int32_t block_id = blockid_of_inode(inode_id);
    int32_t inode_offset = offset_of_inode(inode_id);
    if (disk_read_1K_block(block_id, inode_buf) == -1)
        return -1;
    struct inode *next_inode = (struct inode *)inode_buf;
    next_inode += inode_offset;
    next_inode->link++;
    for(int i = 0;i < 6;i++){
        next_inode->block_point[i] = src_inode.block_point[i];
    }
    if (disk_write_1K_block(block_id, inode_buf) == -1)
        return -1;

    if (disk_read_1K_block(src_inode.block_point[0], block_buf) == -1)
        return -1;
    char self[] = ".";
    inode_id = findItemInDir(src_inode, self);
    block_id = blockid_of_inode(inode_id);
    inode_offset = offset_of_inode(inode_id);
    if (disk_read_1K_block(block_id, inode_buf) == -1)
        return -1;
    next_inode = (struct inode *)inode_buf;
    next_inode += inode_offset;
    next_inode->link++;
    if (disk_write_1K_block(block_id, inode_buf) == -1)
        return -1;
}

