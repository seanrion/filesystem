#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "myfile.h"
#include "disk.h"

extern struct inode currentDirinode;

/* 获得当前目录名 */
void GetDir()
{
    char *path = getPath();
    printf("%s$ ", path);
}

int main()
{

    openSystem();
    initRootDir();

    while(1)
    {
        GetDir();
        fflush(stdout);
        //读取字符串
        char buf[1024];
        int s = read(0, buf, 1024);
        if(s > 0)//有读取到字符
        {
            int i = 0;
            for( i = 0; i < s; ++i)
            {
                if(buf[i] == '\b' && i >= 1)
                {
                    int j = 0;
                    for( j = i+1; j < s; ++j)
                    {
                        buf[j-2] = buf[j];
                    }
                    buf[s-2] = 0;
                    i-=1;
                }
                else if(buf[i] == '\b' && i == 0)
                {
                    int j = 0;
                    for( j = 1; j < s; ++j)
                    {
                        buf[j-1] = buf[j];
                    }
                    buf[s-1] = 0;
                }
                else
                {
                    continue;
                }
            }
            buf[s] = 0;
        }
        else
        {
            continue;
        }
        //将读取到的字符串分成多个字符串
        char* start = buf;
        int i =1;
        char* MyArgv[10] = {0};
        MyArgv[0] = start;
        /* 跳过空格 */
        while(*start)
        {
            if(isspace(*start))
            {
                *start = 0;
                start++;
                MyArgv[i++] = start;
            }
            else
            {
                ++start;
            }
        }
        MyArgv[i-1] = NULL;

        if(!strcmp(MyArgv[0], "ls"))
        {
            showDir();
        }
        else if(!strcmp(MyArgv[0], "cd"))
        {
            if(MyArgv[1] != NULL)
                changeDir(MyArgv[1]);
            else
            {
                printf("usage:cd <dirname>\n");
            }
        }
        else if(!strcmp(MyArgv[0], "touch"))
        {
            if(MyArgv[1] != NULL)
                creatFile(&currentDirinode, MyArgv[1]);
            else
            {
                printf("usage:touch <filename>\n");
            }
        }
        else if(!strcmp(MyArgv[0], "mkdir"))
        {
            if(MyArgv[1] != NULL)
                creatDir(&currentDirinode,MyArgv[1]);
            else
            {
                printf("usage:mkdir <dirname>\n");
            }
        }
        else if(!strcmp(MyArgv[0], "cp"))
        {
            if (MyArgv[1] != NULL && MyArgv[2] != NULL)
                copyFile(MyArgv[1], MyArgv[2]);
            else
            {
                printf("cp <dst path> <src path>\n");
            }
        }
        else if(!strcmp(MyArgv[0], "shutdown"))
        {
            exitSystem();
        }
        else if(!strcmp(MyArgv[0], "help"))
        {
            printf("ls\n");
            printf("touch <filename>\n");
            printf("mkdir <dirname>\n");
            printf("cd <dirname>\n");
            printf("cp <dst path> <src path>\n");
            printf("shutdown\n");
        }
        else
        {
            printf("error:command not found\n");
        }
    }
    return 0;
}
