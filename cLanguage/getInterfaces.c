#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ether.h> 
 
#define MAXINTERFACES 16        // 最大接口数 
 
typedef struct interface{
 char name[20];          //接口名字
 unsigned char ip[4];  //IP地址
 unsigned char mac[6];  //MAC地址
 unsigned char netmask[4]; //子网掩码
 unsigned char br_ip[4];  //广播地址
 int  flag;           //状态
}INTERFACE;
 
int interface_num=0;                    //接口数量
INTERFACE net_interface[MAXINTERFACES]; //接口数据

/**
 * 功能说明：可以在linux系统下，获取接口相关的一些信息，作为平时测试使用的小工具
 * 编译说明：gcc -o getInterfaces getInterfaces.c
 * 使用说明：./getInterfaces
 * **/
int main(int argc,char *argv[])
{
    struct ifreq buf[MAXINTERFACES];    // ifreq结构数组 
    struct ifconf ifc;                  // ifconf结构 
    //  int sock_raw_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));//创建套接字
    int sock_raw_fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ARP));//创建套接字
    char ifname[16];

    /* 初始化ifconf结构 */
    ifc.ifc_len = sizeof(buf);
    ifc.ifc_buf = (caddr_t) buf;

    /* 获得接口列表 */
    if (ioctl(sock_raw_fd, SIOCGIFCONF, (char *) &ifc) == -1)
    {
        perror("SIOCGIFCONF ioctl");
        return -1;
    }
    interface_num = ifc.ifc_len / sizeof(struct ifreq); // 接口数量
    printf("num=%d\n",interface_num);                   //打印接口数量
    char buff[20]="";
    int ip;
    int if_len = interface_num;
    while (if_len-- > 0)   // 遍历每个接口 
    {  
        sprintf(net_interface[if_len].name, "%s", buf[if_len].ifr_name); // 接口名称
        /* 获得接口标志 */
        if (!(ioctl(sock_raw_fd, SIOCGIFFLAGS, (char *) &buf[if_len])))
    {
            /* 获取接口状态 */
            if (buf[if_len].ifr_flags & IFF_UP)
    {
    net_interface[if_len].flag = 1;
            }
            else
    {
    net_interface[if_len].flag = 0;
            }
        }
    else
    {
            char str[256];
            sprintf(str, "SIOCGIFFLAGS ioctl %s", buf[if_len].ifr_name);
            perror(str);
        }

        /* 获取IP地址 */
        if (!(ioctl(sock_raw_fd, SIOCGIFADDR, (char *) &buf[if_len])))
    {
    bzero(buff,sizeof(buff));
    sprintf(buff, "%s", (char*)inet_ntoa(((struct sockaddr_in*) (&buf[if_len].ifr_addr))->sin_addr));
    inet_pton(AF_INET, buff, &ip);
    memcpy(net_interface[if_len].ip, &ip, 4);
    }
    else
    {
            char str[256];
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[if_len].ifr_name);
            perror(str);
        }

        /* 获取子网掩码 */
        if (!(ioctl(sock_raw_fd, SIOCGIFNETMASK, (char *) &buf[if_len])))
    {
    bzero(buff,sizeof(buff));
    sprintf(buff, "%s", (char*)inet_ntoa(((struct sockaddr_in*) (&buf[if_len].ifr_addr))->sin_addr));
    inet_pton(AF_INET, buff, &ip);
    memcpy(net_interface[if_len].netmask, &ip, 4);
        }
    else
    {
            char str[256];
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[if_len].ifr_name);
            perror(str);
        }

        /* 获取广播地址 */
        if (!(ioctl(sock_raw_fd, SIOCGIFBRDADDR, (char *) &buf[if_len])))
    {
    bzero(buff,sizeof(buff));
    sprintf(buff, "%s", (char*)inet_ntoa(((struct sockaddr_in*) (&buf[if_len].ifr_addr))->sin_addr));
    inet_pton(AF_INET, buff, &ip);
    memcpy(net_interface[if_len].br_ip, &ip, 4);
        }
    else
    {
            char str[256];
            sprintf(str, "SIOCGIFADDR ioctl %s", buf[if_len].ifr_name);
            perror(str);
        }

        /*获取MAC地址 */
    if (!(ioctl(sock_raw_fd, SIOCGIFHWADDR, (char *) &buf[if_len])))
    {
    memcpy(net_interface[if_len].mac, (unsigned char *)buf[if_len].ifr_hwaddr.sa_data, 6);
    }
    else
    {
            char str[256];
            sprintf(str, "SIOCGIFHWADDR ioctl %s", buf[if_len].ifr_name);
            perror(str);
        }
    printf("---------------------------------------\n");
    printf("name=%s\n",net_interface[if_len].name);//打印接口名字
    printf("ip=%d.%d.%d.%d\n",net_interface[if_len].ip[0],net_interface[if_len].ip[1],\
    net_interface[if_len].ip[2],net_interface[if_len].ip[3]);//打印IP地址
    printf("mac=%02x:%02x:%02x:%02x:%02x:%02x\n",net_interface[if_len].mac[0],\
    net_interface[if_len].mac[1],net_interface[if_len].mac[2],net_interface[if_len].mac[3],\
    net_interface[if_len].mac[4],net_interface[if_len].mac[5]);//打印MAC地址
    printf("netmask=%d.%d.%d.%d\n",net_interface[if_len].netmask[0],\
    net_interface[if_len].netmask[1],net_interface[if_len].netmask[2],\
    net_interface[if_len].netmask[3]);//打印子网掩码
    printf("br_ip=%d.%d.%d.%d\n",net_interface[if_len].br_ip[0],net_interface[if_len].br_ip[1],\
    net_interface[if_len].br_ip[2],net_interface[if_len].br_ip[3]);//打印广播地址
    printf("flag=%d\n",net_interface[if_len].flag);//打印状态
    }
    return 0;
}