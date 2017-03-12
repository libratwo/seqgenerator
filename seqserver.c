#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <errno.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <signal.h>

/********************************************
 * this is a seqno multi gen&read program   *
 * pointers:                                *
 *  1. p_step   step by step pointer        *
 *  2. p_copy   copy of p_step(stand alone  *
 *              when read(线程级)           *
 *  3. p_fill   pre fill in sequences in arr*
 * static arrays:                           *
 *  1. arr_1    _|  pre store sequences     *
 *  --------    _|- switch for fetch        *
 ********************************************/

/******************************
 * initial modules:           *
 * 1. load from seq file      *
 * 2. map into system memory  *
 * 3. continuously update file*
 ******************************/
/* 单个数组(动态/静态)空间大小
 * 动态实现可配置化
 * 静态实现简单 */
#define MAXBACKLEN      20
#define SEQLEN          6
#define SHMSEQFILE      "shm_seqlst"

/* 简单的日志记录宏 */
#define LOG_DBG(fmt,args...) \
    do { \
        fprintf(stdout, "[DBG] [%s:%d]"fmt"\n",__FILE__,__LINE__,##args); \
    } while (0)

#define LOG_ERR(fmt,args...) \
    do { \
        fprintf(stderr, "[ERR] [%s:%d]"fmt"\n",__FILE__,__LINE__,##args); \
    } while (0)

/* 共享内存指针 */
static void * gpshm = NULL;
/* 序列号缓存buff指针 */
static void * gpbuf = NULL;
/* 文件描述符 */
static int fd = 0;
static int fd_shm; /* 共享内存描述符 */
/* 使用伪循环数组 */
/* static int arr_1[MAXBACKLEN] = {0}; */

/* trap for signal, to make sure atexit funs working */
void sigroutine(int signo)
{
    switch (signo)
    {
        case 1:
            LOG_DBG("Get Signal  --  SIGHUP");
            break;
        case 2:
            LOG_DBG("Get Signal  --  SIGINT");
            break;
        case 3:
            LOG_DBG("Get Signal  --  SIGQUIT");
            break;
        default:
            LOG_ERR("No set signal");
    }

    exit(1);
}

void * mapfile(char *fpath)
{
    int iret = 0;
    char buff[SEQLEN+1] = {0};
    sprintf(buff,"%0*d\0",SEQLEN,0);
    mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH; /* 0666 */
    /* 初始化序列文件 */
    fd = open(fpath,O_RDWR|O_CREAT|O_EXCL,mode);
    if(fd == -1 && errno == EEXIST)
    {
        /* 已经存在序列文件, 直接打开 */
        LOG_ERR("[open seq] %s", strerror(errno));
        fd = open(fpath,O_RDWR,mode);
        if (fd == -1)
        {
            LOG_ERR("[reopen seq] %s", strerror(errno));
            return NULL;
        }
        if (read(fd, buff, SEQLEN) == -1)
        {
            LOG_ERR("[read seq] %s", strerror(errno));
            close(fd);
            return NULL;
        }
        lseek(fd,0,SEEK_SET);
    }
    else if (fd == -1)
    {
        /* open 错误 */
        LOG_ERR("[open seq] %s", strerror(errno));
        return NULL;
    }
    else
    {
        /* 需要初始化 */
        if (write(fd,buff,SEQLEN+1) == -1)
        {
            LOG_ERR("[init seq] %s", strerror(errno));
            close(fd);
            return NULL;
        }
        /* reset offset */
        lseek(fd,0,SEEK_SET);
    }
    /* ->Here fd is valid */
    fd_shm = shm_open(SHMSEQFILE,O_RDWR|O_CREAT|O_TRUNC,mode);
    if (fd_shm == -1)
    {
        LOG_ERR("[shm creat] %s", strerror(errno));
        return NULL;
    }
    /* 初始化shm空间 */
    if(ftruncate(fd_shm, MAXBACKLEN) == -1)
    {
        LOG_ERR("[shm init] %s", strerror(errno));
        return NULL;
    }

    gpshm = mmap(NULL,SEQLEN+1,PROT_READ|PROT_WRITE,MAP_SHARED,fd_shm,0);
    if (gpshm == MAP_FAILED)
    {
        LOG_ERR("[mmap shm] %s", strerror(errno));
        gpshm = NULL;
    }
    close(fd_shm);

    memcpy(gpshm, buff, SEQLEN);

    return gpshm;
}

/* 刷新文件函数 */
void flushfile(void)
{
    LOG_DBG("Flush memory into seq file..");
    if (gpshm == NULL)
        return;

    /* msync(gpshm,SEQLEN+1,MS_SYNC); */
    if (write(fd, gpshm, SEQLEN) == -1)
        LOG_ERR("[flush seq] %s", strerror(errno));
    lseek(fd, 0, SEEK_SET);
}

void unmapfile(void)
{
    LOG_DBG("Release shmemory...");
    if (gpshm == NULL);
    return;

    close(fd);
    munmap(gpshm,SEQLEN+1);
    shm_unlink(SHMSEQFILE);
}

/* 建立seq缓冲区(arr_1动态实现) */
int * createbuf(int size)
{
    /* int * gpbuf */
    gpbuf = malloc(sizeof(int)*size);
    if (gpbuf == NULL)
        strerror(errno);
    return gpbuf;
}

/* 释放缓冲区 */
void freebuf(void)
{
    LOG_DBG("Free Buff..");
    if (gpbuf == NULL)
        return;
    free(gpbuf);
}

int main(int argc, char *argv[])
{
    char fpath[] = "seq.lst";
    char buff[SEQLEN+1];
    char lastbuf[SEQLEN+1];
    memset(buff, 0x0, sizeof(buff));
    memset(lastbuf, 0x0, sizeof(lastbuf));

    /* 设置为守护进程 */
    /* daemon(0,0); */

    LOG_DBG("setting signal..");
    signal(SIGINT,sigroutine);
    signal(SIGHUP,sigroutine);
    signal(SIGQUIT,sigroutine);

    /* 注册退出的清理函数
     * -. 刷新seq到文件
     * 1. 释放缓冲区(动态)
     * 2. 释放共享内存区(刷新)
     */
    LOG_DBG("register exit functions..");
    atexit(unmapfile);
    atexit(flushfile);
    atexit(freebuf);

    LOG_DBG("Load seq file into memory..");
    mapfile(fpath);
    if (gpshm == NULL)
        return -1;
    LOG_DBG("map shared addr [0x%p]", gpshm);

    createbuf(MAXBACKLEN);
    if (gpbuf == NULL)
    {
        return -1;
    }
    LOG_DBG("malloc buff addr [0x%p]", gpbuf);

    while (1)
    {
        memcpy(buff, gpshm, SEQLEN);
        if (memcmp(buff, lastbuf, SEQLEN))
        {
            memcpy(lastbuf, buff, SEQLEN);
            flushfile();
        }
        sleep(3);
    }

    return 0;
}

