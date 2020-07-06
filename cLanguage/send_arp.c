#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdarg.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>
// #include "adapter.h"
#include <stdbool.h>

struct arp_msg {
    /* Ethernet header */
    uint8_t h_dest[ 6] ; /* 00 destination ether addr */
    uint8_t h_source[ 6] ; /* 06 source ether addr */
    uint16_t h_proto; /* 0c packet type ID field */

    /* ARP packet */
    uint16_t htype; /* 0e hardware type (must be ARPHRD_ETHER) */
    uint16_t ptype; /* 10 protocol type (must be ETH_P_IP) */
    uint8_t hwAddrLen; /* 12 hardware address length (must be 6) */
    uint8_t protoAddrLen; /* 13 protocol address length (must be 4) */
    uint16_t operation; /* 14 ARP opcode */
    uint8_t srcMac[ 6] ; /* 16 sender's hardware address */
    uint8_t srcIP[ 4] ; /* 1c sender's IP address */
    uint8_t dstMac[ 6] ; /* 20 target's hardware address */
    uint8_t dstIP[ 4] ; /* 26 target's IP address */
    // uint8_t pad[ 18] ; /* 2a pad for min. ethernet payload (60 bytes) */
}PACKED;

bool g_system_running=true;
uint32_t local_ip;
uint8_t local_mac[ 6 ];
uint32_t find_ip = 0;
pthread_mutex_t g_cfglock;
volatile uint32_t g_mac_count=0;
char (*mac_list)[18];
int  mac_list_max=0;
uint8_t raw_mac[64][6];

int setsockopt_broadcast( int fd)
{
    const int const_int_1 =1;
    return setsockopt ( fd,SOL_SOCKET,SO_BROADCAST, & const_int_1, sizeof ( const_int_1) ) ;
}

void prmac(u_char *ptr)
{
    printf("MAC is: %02x:%02x:%02x:%02x:%02x:%02x\n",*ptr,*(ptr+1),*(ptr+2),*(ptr+3),*(ptr+4),*(ptr+5));
}

void printIP(const char *msg, uint8_t* ip)
{
    printf("%s IP:%d.%d.%d.%d\n",msg,ip[0],ip[1],ip[2],ip[3]);
}

 int send_arp( uint32_t test_ip, uint32_t from_ip, uint8_t * my_mac, const char * interface)
{
     int sock_send;
     int rv =1; /* "no reply received" yet */
     struct sockaddr addr; /* for interface name */
     struct arp_msg arp;
     sock_send = socket ( PF_PACKET , SOCK_PACKET , htons ( ETH_P_ARP) ) ;

     if ( sock_send == - 1) {
         perror ( "raw_socket" ) ;
         return - 1;
     }

     if ( setsockopt_broadcast( sock_send ) == - 1) {
         perror ( "cannot enable bcast on raw socket" ) ;
         goto ret;
     }

      /* send arp request */
     memset ( & arp,0, sizeof ( arp) ) ;
     memset ( arp. h_dest,0xff,6) ; /* MAC DA */
     memcpy ( arp. h_source,my_mac,6) ; /* MAC SA */
     arp. h_proto = htons ( ETH_P_ARP) ; /* protocol type (Ethernet) */
     arp. htype = htons ( ARPHRD_ETHER) ; /* hardware type */
     arp. ptype = htons ( ETH_P_IP) ; /* protocol type (ARP message) */
     arp. hwAddrLen =6; /* hardware address length */
     arp. protoAddrLen =4; /* protocol address length */
     arp. operation = htons ( ARPOP_REQUEST) ; /* ARP op code */
     memcpy ( arp. srcMac,my_mac,6) ; /* source hardware address */
     memcpy ( arp. srcIP, & from_ip, sizeof ( from_ip) ) ; /* source IP address */
     memcpy ( arp. dstIP, & test_ip, sizeof ( test_ip) ) ; /* target IP address */
     memset ( & addr,0, sizeof ( addr) ) ;

     strncpy( addr. sa_data,interface, sizeof ( addr. sa_data) ) ;
     if ( sendto ( sock_send, & arp, sizeof ( arp) ,0, & addr, sizeof ( addr) ) <0) {
            perror("sendto error");
        }  
ret:
    close (sock_send) ;
     return rv;
}

 int read_interface( const char * interface, int * i, uint32_t * addr, uint8_t * arp)
{
    int fd;
    struct ifreq ifr;
    struct sockaddr_in * our_ip;

    memset ( & ifr,0, sizeof ( ifr) ) ;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr. ifr_addr. sa_family = AF_INET ;
    strncpy( ifr.ifr_name,interface, sizeof(ifr.ifr_name) );

    if ( arp) 
    {
        if ( ioctl( fd, SIOCGIFHWADDR , & ifr) !=0) 
        {
            printf("SIOCGIFHWADDR error\n");
            close ( fd) ;
            return - 1;
        }

        memcpy ( arp,ifr. ifr_hwaddr. sa_data,6) ;
        prmac ( arp ) ;
    }

    if ( addr) {
        if ( ioctl( fd, SIOCGIFADDR , & ifr) !=0) {
            perror ( "ioctl" ) ;
            close ( fd) ;
            return - 1;
        }

        our_ip = ( struct sockaddr_in * ) & ifr. ifr_addr;
        * addr =our_ip-> sin_addr. s_addr;
    }

    close ( fd);
    return 0;
}

 void* thread_read_arp(void* param)

{
    struct sockaddr repsa;
    socklen_t salen, recved_len ;
    struct arp_msg response;
    int recvfd = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP));
    bzero(&response, sizeof(response));
    bzero(&repsa, sizeof(repsa));
    salen = sizeof(repsa);

    while(g_system_running){
            if((recved_len = recvfrom(recvfd, &response, sizeof(response), 0, (struct sockaddr *)&repsa, &salen)) <= 0) {
                perror("Recvfrom error");
                break;
            }

            printf("Response %d bytes received\n",recved_len);
            printIP("response", response.srcIP);

            if( memcmp(response.dstIP, &local_ip,  4 ) ==0
                && memcmp(response.dstMac, local_mac, 6 ) ==0){
                        printf("get a data\n");
            }
    }
    close(recvfd);
}

int arp_scaner_init( const char * interface )
{
    pthread_t tid;
    read_interface( interface, NULL, &local_ip, local_mac);
    printf("local IP");
    printIP("", (uint8_t*)&local_ip );
    pthread_mutex_init(&g_cfglock, NULL);
    // ipc_createthread(&tid, thread_read_arp, NULL, "arp-reading");
    // send_arp( find_ip,local_ip,local_mac,interface);
    send_arp( local_ip, local_ip, local_mac, interface);
}

int main(int argc, char **argv)
{
    for(int i=1; i<argc; i++)
    {
        arp_scaner_init(argv[i]);
    }

    return 0;
}