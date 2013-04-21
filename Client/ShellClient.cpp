/*
  ������ windows �ϵĿͻ���

  ʹ�÷���  

  icmp_shell.exe [ip]
  Ȼ�󵯳�һ�� ���� ����ִ������

  ����Ի�������Ƕ���� 
*/

#include <WinSock2.h>
#include<ws2tcpip.h> 
#include <Windows.h>
#include <stdio.h>
#include <mstcpip.h>
#include "resource.h"
#include "shelldlg.h"

#pragma comment(lib,"ws2_32")

#define dbg_msg(fmt,...) do{\
    printf(##__FUNCTION__##" %d :"##fmt,__LINE__,__VA_ARGS__);\
    }while(0);

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

#pragma pack(push,1)

struct icmphdr
{
    uint8 type;		/* message type */
    uint8 code;		/* type sub-code */
    uint16 checksum;
    union
    {
        struct
        {
            uint16	id;
            uint16	sequence;
        } echo;			/* echo datagram */
        uint32	gateway;	/* gateway address */
        struct
        {
            uint16	__unused;
            uint16	mtu;
        } frag;			/* path mtu discovery */
    } un;
};

typedef struct ip_address
{
    u_char byte1;
    u_char byte2;
    u_char byte3;
    u_char byte4;
}ip_address;


typedef struct _IPHeader		// 20�ֽڵ�IPͷ
{
    uint8     iphVerLen;      // �汾�ź�ͷ���ȣ���ռ4λ��
    uint8     ipTOS;          // �������� 
    uint16    ipLength;       // ����ܳ��ȣ�������IP���ĳ���
    uint16    ipID;			  // �����ʶ��Ωһ��ʶ���͵�ÿһ�����ݱ�
    uint16    ipFlags;	      // ��־
    uint8     ipTTL;	      // ����ʱ�䣬����TTL
    uint8     ipProtocol;     // Э�飬������TCP��UDP��ICMP��
    uint16    ipChecksum;     // У���
    union {
        unsigned int   ipSource;
        ip_address ipSourceByte;
    };
    union {
        unsigned int   ipDestination;
        ip_address ipDestinationByte;
    };
} IPHeader, *PIPHeader; 

#pragma pack(pop)

SOCKET g_sock;


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

/*
����һ���ظ��� 
*/
BOOL  send_icmp_replay_packet(struct icmphdr *request,uint32 ip,char *data,int size)
{
    BOOL ret = FALSE;
    int  packet_size = sizeof(struct icmphdr) + size;
    struct icmphdr *reply_packet = (struct icmphdr *)malloc(packet_size);
    if(NULL != reply_packet)
    {
        char *pdata ;
        sockaddr_in addr;

        memcpy(reply_packet,request,sizeof(struct icmphdr));
        reply_packet->type = 0 ; // reply
        pdata = (char *)(reply_packet + 1);
        memcpy(pdata,data,size);

        reply_packet->checksum = 0;
        reply_packet->checksum = checksum((unsigned short *)reply_packet,packet_size);

        memset(&addr,0,sizeof(sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_addr.S_un.S_addr = ip;
        if(sendto(g_sock,(char *)reply_packet,packet_size,0,(sockaddr *)&addr,sizeof(sockaddr_in)) < 1)
        {
            dbg_msg("send packet failed !! \n");
        }
    }
    return ret;
}

BOOL set_socket_recv_all(SOCKET s)
{
    //http://www.okob.net/wp/index.php/2009/04/30/icmp-over-raw-sockets-under-windows-vista/
    /* Run the IOCTL that disables packet filtering on the socket. */
    DWORD tmp, prm = RCVALL_IPLEVEL; /* "RCVALL_IPLEVEL" (Vista SDK) */
    if(WSAIoctl(s, SIO_RCVALL, &prm, sizeof(prm), NULL, 0,
        &tmp, NULL, NULL) == SOCKET_ERROR)
    {
        /* Handle error here */
        return FALSE;
    }
    return TRUE;
}

int main(int argc,char **argv)
{
    WSAData wsa;
    sockaddr_in addr;
    char buff[1500];
    int nbytes = 0;
    int opt = 1;

    int len = sizeof(sockaddr);

    WSAStartup(MAKEWORD(2,2),&wsa);
    g_sock = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
    if(INVALID_SOCKET == g_sock)
    {
        dbg_msg("create socket failed \n");
        return -1;
    }

    //setsockopt(g_sock,IPPROTO_ICMP, IP_HDRINCL,(char *)&opt,sizeof(int));  //��Ҫ���� ip ͷ��ʱ�����Ҫ����
    
    addr.sin_addr.S_un.S_addr = inet_addr("192.168.91.1");
    addr.sin_family = AF_INET;
    addr.sin_port = 0;

    if(SOCKET_ERROR == bind(g_sock,(sockaddr *)&addr,sizeof(sockaddr)))
    {
        dbg_msg("bind failed");
        closesocket(g_sock);
        return -1;
    }
    
    set_socket_recv_all(g_sock);  //  vista ���ϵ�ϵͳ�������������� �����յ� ping request ��������

    while((nbytes = recv(g_sock,buff,1500,0)) > 0) //�ΰ�  ֻ���յ� reply �İ�
    {
        IPHeader *ip = (IPHeader *)buff;
        int iplen = (ip->iphVerLen & 0xf) * sizeof(unsigned int);
        struct icmphdr *icmp = (struct icmphdr *)((char *)ip + iplen);
        printf("recv %d bytes from %s \n",nbytes,inet_ntoa(*(in_addr *)&ip->ipSource));
        if(0 == icmp->type)
        {
            printf("icmp reply \n");
        }
        else if(8 == icmp->type)
        {
            printf("icmp request \n");
            //�յ���������ʱ�� ����Ӧ�÷��ͱ��ص�����  ������ľ�� 
            send_icmp_replay_packet(icmp,ip->ipSource,"I am sincoder",8);
        }
        else
        {
            printf("unknown type : %d \n",icmp->type);
        }
    }
    dbg_msg("last error : %d \n",GetLastError());
    closesocket(g_sock);
	return 0;
}
