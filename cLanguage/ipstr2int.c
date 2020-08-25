#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
// #include <math.h>
#include <arpa/inet.h>

/* 192.168.34.232 -> 3232244456 */
unsigned int ipstr2int(const char *ip)
{
    struct in_addr in_addr1;
    inet_aton(ip, &in_addr1);

    return in_addr1.s_addr;
}

/* 3232244456 -> 192.168.34.232 */
char * int2ipstr(unsigned int ip_int)
{
	char *ipstr = NULL;
    struct in_addr in_addr1;

    in_addr1.s_addr = ip_int;
    ipstr = inet_ntoa(in_addr1);

    return ipstr;
}

/**
 * 功能说明：可以将ip地址的点分表示转换成整形值或者相反的转换
 * 编译说明：gcc -o ipstr2int ipstr2int.c -lm
 * 使用说明：./ipstr2int 
 * **/
int main(int argc, char *argv[])
{
    const char *ip = "192.168.186.15";
    char *ipstr = NULL;
    unsigned int ip_add = 0;

    ip_add = ipstr2int(ip);	
    printf("%u\n",ip_add);

    ipstr = int2ipstr(ip_add);
    printf("%s\n",ipstr);

    return 0;
}