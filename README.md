# filesystem
简单的文件系统实现

实现青春版Ext2文件系统，Ext2文件系统将盘块分成两大类：保存元数据（管理数据）的元数据盘块，以及存放文件内容数据的数据盘块。 

superblock ：超级块，包含整个系统的总体信息
inode ：记录着文件的元数据，每个文件都与一个inode对应，但一个inode可能对应多个文件（硬链接）。在 本实验中可以认为一个inode对应一个文件。
数据块 ：记录文件内容

超级块是文件系统的起点系统，记录了文件系统全局性的一些信息。

typedef struct super_block {
    int32_t magic_num;                  // 幻数
    int32_t free_block_count;           // 空闲数据块数
    int32_t free_inode_count;           // 空闲inode数
    int32_t dir_inode_count;            // 目录inode数
    uint32_t block_map[128];            // 数据块占用位图
    uint32_t inode_map[32];             // inode占用位图
} sp_block;

对于每个文件，都会有一个inode结构体包含其元数据（文件类型，文件大小）等。
struct inode {
    uint32_t size;              // 文件大小
    uint16_t file_type;         // 文件类型（文件/文件夹）
    uint16_t link;              // 连接数
    uint32_t block_point[6];    // 数据块指针
};





文件夹（目录）用于将整个文件系统的文件行程按路径名访问的树形组织结构排列。Ext2文件系统并没有单独形成用于目录数据的数据块，而是将目录与普通文件一样存放在数据块中。
struct dir_item {               // 目录项一个更常见的叫法是 dirent(directory entry)
    uint32_t inode_id;          // 当前目录项表示的文件/目录的对应inode
    uint16_t valid;             // 当前目录项是否有效 
    uint8_t type;               // 当前目录项类型（文件/目录）
    char name[121];             // 目录项表示的文件/目录的文件名/目录名
};

系统结构参考ext2系统，模拟了文件系统，并能完成如下功能：
1.创建文件/文件夹（数据块可预分配）；
2.读取文件夹内容；
3.复制文件；
4.关闭系统；
5.系统关闭后，再次进入该系统还能还原出上次关闭时系统内的文件部署。

为实现的文件系统实现简单的 shell 以及 shell 命令以展示实现的功能。 可参考实现以下shell命令：
ls - 展示读取文件夹内容
mkdir - 创建文件夹
touch - 创建文件
cp - 复制文件
shutdown - 关闭系统

