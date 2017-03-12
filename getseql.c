#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>

#include <sys/mman.h>

/* 简单的日志记录宏 */
#define LOG_DBG(fmt,args...) \
    do { \
        fprintf(stdout, "[DBG] [%s:%d]"fmt"\n",__FILE__,__LINE__,##args); \
    } while (0)

#define LOG_ERR(fmt,args...) \
    do { \
        fprintf(stderr, "[ERR] [%s:%d]"fmt"\n",__FILE__,__LINE__,##args); \
    } while (0)

#define SEQLEN      6
#define MAXBUFF     64
#define SHMSEQFILE  "shm_seqlst"

static void * gpshm = NULL;

struct flock *file_lock(short type, short whence)
{
    static struct flock ret;
    ret.l_type = type;
    ret.l_start = 0;
    ret.l_whence = whence;
    ret.l_len = 0;
    ret.l_pid = getpid();

    return &ret;
}

int exclusive_lock(int fd)
{
    int iret;
    iret = fcntl(fd, F_SETLKW, file_lock(F_WRLCK, SEEK_SET));
    if (iret == -1 && errno == EINTR)
    {
        iret = fcntl(fd, F_SETLKW, file_lock(F_WRLCK, SEEK_SET));
    }

    return iret;
}

int exclusive_unlock(int fd)
{
    return fcntl(fd, F_SETLK, file_lock(F_UNLCK, SEEK_SET));
}

char *increbuf(const char *rbuf,int rlen,int wlen)
{
    /* 读出数字串长, 写入数字串长 */
    static char buff[MAXBUFF+1] = {[0 ... MAXBUFF] = '0'};
    int i,j,c;
    int npp = 1;

    for (i=rlen-1,j=wlen-1;i>=0&&j>=0;i--,j--)
    {
        c = *(rbuf+i)+npp;
        if (c>'9')
        {
            npp = 1;
            *(buff+j) = '0';
        }
        else
        {
            npp = 0;
            *(buff+j) = c;
        }
    }
    buff[wlen] = 0x0;
    return buff;
}

char *increbufl(char *rbuf,int rlen,int wlen)
{
    /* 读出数字串长, 写入数字串长 */
    int i,j,c;
    int npp = 1;

    for (i=rlen-1,j=wlen-1;i>=0&&j>=0;i--,j--)
    {
        c = *(rbuf+i)+npp;
        if (c>'9')
        {
            npp = 1;
            *(rbuf+j) = '0';
        }
        else
        {
            npp = 0;
            *(rbuf+j) = c;
        }
    }
    return rbuf;
}

int main(int argc, char *argv[])
{
    char rbuf[MAXBUFF+1] = {0};
    char *wbuf = NULL;
    int nread = 0;
    int fd = -1;

    mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH; /* 0666 */

    fd = shm_open(SHMSEQFILE, O_RDWR, mode);
    if (fd == -1)
    {
        LOG_ERR("[shm_open] %s", strerror(errno));
        return -1;
    }

    exclusive_lock(fd);
#if 0
    /* read num */
    nread = read(fd, rbuf, 6);

    wbuf = increbuf(rbuf, nread, 6);
    fprintf(stdout,"%s", wbuf);
    fflush(stdout);

    lseek(fd, 0, SEEK_SET);
    write(fd, wbuf, 6);
#endif
    gpshm = mmap(NULL, SEQLEN+1, PROT_READ|PROT_WRITE, MAP_SHARED, fd ,0);
    if (gpshm == MAP_FAILED)
    {
        strerror(errno);
        return -1;
    }
    /* LOG_DBG("map shared addr [0x%p]", gpshm); */
    close(fd);

    increbufl(gpshm, 6, 6);
    write(STDOUT_FILENO,gpshm,6);
    exclusive_unlock(fd);
    munmap(gpshm, SEQLEN+1);

    return 0;
}

