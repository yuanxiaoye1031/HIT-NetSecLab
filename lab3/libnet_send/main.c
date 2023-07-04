#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libnet.h>

int main(int argc, char *argv[]){
    int res = 0;
    char send_msg[1000] = "";
    char err_buf[100] = "";
    libnet_t *lib_net = NULL;
    int lens = 0;
    libnet_ptag_t lib_t = 0;
    unsigned char src_mac[6] = {0x00,0x0c,0x29,0x7a,0x2b,0x9f};     //发送者网卡地址00:0c:29:7a:2b:9f
    unsigned char dst_mac[6] = {0x00,0x0c,0x29,0xf1,0x3a,0x0b};     //接收者网卡地址00:0c:29:f1:3a:0b
    char *src_ip_str = "192.168.58.130";      //源主机IP地址
    char *dst_ip_str = "192.168.58.129";      //目的主机IP地址
    unsigned long src_ip, dst_ip = 0;

    char str[1000]="";
    printf("请输入发送内容:");
    scanf("%s",str);
    printf("发送内容：%s\n",str);
    lens = sprintf(send_msg, "%s", str);


//    printf("发送内容：%s\n",str);
//    printf("发送长度: %d\n",lens);
    fflush(stdin);

    //初始化
    lib_net = libnet_init(LIBNET_LINK_ADV, "ens33", err_buf);
    if(lib_net == NULL){
        perror("libnet 初始化失敗!\n");
        return -1;
    }

    //将字符串类型的ip转换为顺序网络字节流
    src_ip = libnet_name2addr4(lib_net,src_ip_str,LIBNET_RESOLVE);
    dst_ip = libnet_name2addr4(lib_net,dst_ip_str,LIBNET_RESOLVE);

    //构造udp数据包
    lib_t = libnet_build_udp(10310, 10310, 8+lens, 0, send_msg, lens, lib_net, 0);

    //构造ip数据包
    lib_t = libnet_build_ipv4(20+8+lens, 0, 500, 0, 10, 17, 0, src_ip, dst_ip, NULL, 0, lib_net, 0);

    //构造以太网数据包
    //0x800 或者，ETHERTYPE_IP
    lib_t = libnet_build_ethernet((u_int8_t *)dst_mac, (u_int8_t *)src_mac, 0x800, NULL, 0, lib_net, 0);

    //发送数据包
    res = libnet_write(lib_net);
    if(res == -1){
        perror("libnet发送失败!\n");
        return -1;
    }

    //销毁资源
    libnet_destroy(lib_net);

    printf("数据包发送完成!\n");
    return 0;
}

// gcc -o test main.c -lnet