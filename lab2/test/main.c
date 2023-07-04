#include <pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <netinet/in.h>

// gcc -g -Wall -o test main.c -lpcap

#define BUFSIZ 65535

#define ETHERNET_ADDR_LEN 6
#define IP_ADDR_LEN 4

struct ethernet
{
    u_char dst_mac_addr[ETHERNET_ADDR_LEN]; //目的主机MAC地址
    u_char src_mac_addr[ETHERNET_ADDR_LEN];
    u_char pro_type[2]; //协议类型
};
struct ip
{
    u_char version_hlen; //版本号+头部长度 1字节
    u_char service;      //服务类型
    u_short total_len;   //总长度
    u_short id;          //标识
    u_short flag_shift;  //标志+片偏移
    u_char ttl;
    u_char protocol; //协议
    u_short checksum;
    u_char src_ip_addr[IP_ADDR_LEN];
    u_char dst_ip_addr[IP_ADDR_LEN];
};
struct tcp
{
    u_short src_port;
    u_short dst_port;
    u_int seq;
    u_int ack;
    u_char headlen; //高四位是数据偏移即头长度，后4位保留
    u_char flag;    //高2bit为保留，低6bit为标志
    u_short win;    //窗口大小
    u_short checksum;
    u_short urp; //紧急指针(urgent pointer)
};
struct udp
{
    u_short src_port;
    u_short dst_port;
    u_short len; //总长度
    u_short checksum;
};

struct udpData
{
    struct udp;
    u_char data[1472];
};

void process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *buffer);

int main()
{
    char errbuf[PCAP_ERRBUF_SIZE];  // 错误信息缓冲区
    pcap_t *handle;
    struct bpf_program filter;
    char filter_exp[] = "udp port 10310";
//    char filter_exp[] = "udp or tcp";
    bpf_u_int32 mask;
    bpf_u_int32 net;
    const u_char *packet;
    struct pcap_pkthdr header;
    int num_packets=4;
    pcap_dumper_t *dumper; //定义pcap_dumper_t类型的变量

    pcap_if_t *alldevs;
    pcap_findalldevs(&alldevs, errbuf); // 查找所有网络设备信息

    // 遍历设备
    for(pcap_if_t *pdev = alldevs;pdev;pdev=pdev->next)
    {
        // 找到ens33网卡
        if(strcmp(pdev->name,"ens33")==0)
        {
            const char* device=pdev->name;

            // 查询网络号和子网掩码
            if(pcap_lookupnet(device,&net,&mask,errbuf) == -1)
            {
                // 打印错误信息
                fprintf(stderr, "找不到子网和掩码 %s: %s\n", device, errbuf);
                net = 0;
                mask = 0;
            }

            // 设置混杂模式(只捕获自己包可以不设置)
            // 第三个参数为0是非混杂模式，其他值是混杂模式
            handle = pcap_open_live(device,BUFSIZ,0,1000,errbuf);
            if (handle == NULL) {
                fprintf(stderr, "设备无法打开 %s: %s\n", device, errbuf);
                return(2);
            }

            // 编译并设置过滤器
            if (pcap_compile(handle, &filter, filter_exp, 0, net) == -1) {
                fprintf(stderr, "过滤器解析失败 %s: %s\n", filter_exp, pcap_geterr(handle));
                return(2);
            }
            if (pcap_setfilter(handle, &filter) == -1) {
                fprintf(stderr, "过滤器设置失败 %s: %s\n", filter_exp, pcap_geterr(handle));
                return(2);
            }

            dumper = pcap_dump_open(handle, "capture.pcap"); //打开输出文件
            pcap_loop(handle, num_packets, process_packet, (u_char *)dumper); //将pcap_dumper_t类型的指针作为传入参数
            pcap_dump_close(dumper); //关闭文件
            pcap_close(handle);
            break;
        }
    }
    return 0;
}

void save_ethernet(struct ethernet* eth)
{
    printf("-----以太网首部-----\n");
    printf("源MAC:\t%02x-%02x-%02x-%02x-%02x-%02x\n",eth->src_mac_addr[0],eth->src_mac_addr[1],eth->src_mac_addr[2],eth->src_mac_addr[3],eth->src_mac_addr[4],eth->src_mac_addr[5]);
    printf("目的MAC:\t%02x-%02x-%02x-%02x-%02x-%02x\n",eth->dst_mac_addr[0],eth->dst_mac_addr[1],eth->dst_mac_addr[2],eth->dst_mac_addr[3],eth->dst_mac_addr[4],eth->dst_mac_addr[5]);
//    printf("协议类型:\t0x%02x%02x\n",eth->pro_type[0],eth->pro_type[1]);
    printf("--------------------\n\n");

//    FILE *eth_file = fopen("eth.txt","a");
//    fprintf(eth_file,"源MAC: %02x-%02x-%02x-%02x-%02x-%02x\n",eth->src_mac_addr[0],eth->src_mac_addr[1],eth->src_mac_addr[2],eth->src_mac_addr[3],eth->src_mac_addr[4],eth->src_mac_addr[5]);
//    fprintf(eth_file,"目的MAC: %02x-%02x-%02x-%02x-%02x-%02x\n",eth->dst_mac_addr[0],eth->dst_mac_addr[1],eth->dst_mac_addr[2],eth->dst_mac_addr[3],eth->dst_mac_addr[4],eth->dst_mac_addr[5]);
//    fprintf(eth_file,"协议类型:0x%02x%02x\n\n\n",eth->pro_type[0],eth->pro_type[1]);
//    fclose(eth_file);

//    FILE *fp = fopen("myCapture.txt","a");
//    fprintf(fp,"源MAC: %02x-%02x-%02x-%02x-%02x-%02x\n",eth->src_mac_addr[0],eth->src_mac_addr[1],eth->src_mac_addr[2],eth->src_mac_addr[3],eth->src_mac_addr[4],eth->src_mac_addr[5]);
//    fprintf(fp,"目的MAC: %02x-%02x-%02x-%02x-%02x-%02x\n",eth->dst_mac_addr[0],eth->dst_mac_addr[1],eth->dst_mac_addr[2],eth->dst_mac_addr[3],eth->dst_mac_addr[4],eth->dst_mac_addr[5]);
//    fclose(fp);
}

void save_ip(struct ip* ip)
{

//    u_char version_hlen; //版本号+头部长度 1字节
//    u_char service;      //服务类型
//    u_short total_len;   //总长度
//    u_short id;          //标识
//    u_short flag_shift;  //标志+片偏移
//    u_char ttl;
//    u_char protocol; //协议
//    u_short checksum;
//    u_char src_ip_addr[IP_ADDR_LEN];
//    u_char dst_ip_addr[IP_ADDR_LEN];
//    FILE *ip_file = fopen("ip.txt","a");
//    fclose(ip_file);

    printf("------IP首部------\n");
    printf("源IP地址:\t%d.%d.%d.%d\n",ip->src_ip_addr[0],ip->src_ip_addr[1],ip->src_ip_addr[2],ip->src_ip_addr[3]);
    printf("目的IP地址:\t%d.%d.%d.%d\n",ip->dst_ip_addr[0],ip->dst_ip_addr[1],ip->dst_ip_addr[2],ip->dst_ip_addr[3]);
//    printf("版本号:\t\t%x\n",ip->version_hlen>>4);
//    printf("首部长度:\t%d 字节\n",(ip->version_hlen&0x0F)*4);
//    printf("服务类型:\t0x%x\n",ip->service);
//    printf("IP包总长度:\t%d 字节\n",ip->total_len);
//    printf("标识ID:\t\t%d\n",ip->id);
//    printf("标志位:\t\t%x%x%x\n",(ip->flag_shift&0x8000)>>15,(ip->flag_shift&0x4000)>>14,(ip->flag_shift&0x2000)>>13);
//    printf("段偏移量:\t%d\n",(ip->flag_shift&0x1FFF));
//    printf("TTL:\t\t%d\n",ip->ttl);
//    printf("协议:\t\t%d\n",ip->protocol);
//    printf("校验和:\t\t0x%016x\n",ip->checksum);
    printf("------------------\n\n");

//    FILE *ip_file = fopen("ip.txt","a");
//    fprintf(ip_file,"源IP地址:\t%d.%d.%d.%d\n",ip->src_ip_addr[0],ip->src_ip_addr[1],ip->src_ip_addr[2],ip->src_ip_addr[3]);
//    fprintf(ip_file,"目的IP地址:\t%d.%d.%d.%d\n",ip->dst_ip_addr[0],ip->dst_ip_addr[1],ip->dst_ip_addr[2],ip->dst_ip_addr[3]);
//    fprintf(ip_file,"版本号:\t\t%x\n",ip->version_hlen>>4);
//    fprintf(ip_file,"首部长度:\t%d 字节\n",(ip->version_hlen&0x0F)*4);
//    fprintf(ip_file,"服务类型:\t0x%x\n",ip->service);
//    fprintf(ip_file,"IP包总长度:\t%d 字节\n",ip->total_len);
//    fprintf(ip_file,"标识ID:\t\t%d\n",ip->id);
//    fprintf(ip_file,"标志位:\t\t%x%x%x\n",(ip->flag_shift&0x8000)>>15,(ip->flag_shift&0x4000)>>14,(ip->flag_shift&0x2000)>>13);
//    fprintf(ip_file,"段偏移量:\t%d\n",(ip->flag_shift&0x1FFF));
//    fprintf(ip_file,"TTL:\t\t%d\n",ip->ttl);
//    fprintf(ip_file,"协议:\t\t%d\n",ip->protocol);
//    fprintf(ip_file,"校验和:\t\t0x%016x\n\n\n",ip->checksum);
//    fclose(ip_file);

    FILE *fp = fopen("myCapture.txt","a");
    fprintf(fp,"源IP地址:\t%d.%d.%d.%d\n",ip->src_ip_addr[0],ip->src_ip_addr[1],ip->src_ip_addr[2],ip->src_ip_addr[3]);
    fprintf(fp,"目的IP地址:\t%d.%d.%d.%d\n",ip->dst_ip_addr[0],ip->dst_ip_addr[1],ip->dst_ip_addr[2],ip->dst_ip_addr[3]);
    fclose(fp);
}

void save_tcp(struct tcp* tcp)
{
//    u_short src_port;
//    u_short dst_port;
//    u_int seq;
//    u_int ack;
//    u_char headlen; //高四位是数据偏移即头长度，后4位保留
//    u_char flag;    //高2bit为保留，低6bit为标志
//    u_short win;    //窗口大小
//    u_short checksum;
//    u_short urp; //紧急指针(urgent pointer)

    printf("------TCP首部------\n");
    printf("源端口:\t\t%u\n",tcp->src_port);
    printf("目的端口:\t%u\n",tcp->dst_port);
//    printf("Seq:\t\t%u\n",tcp->seq);
//    printf("Ack:\t\t%u\n",tcp->ack);
//    printf("首部长度:\t%u 字节\n",((tcp->headlen&0xF0)>>4)*4);
//    printf("标志位:\t\t%x%x%x%x%x%x\n",(tcp->flag&0x20)>>5,(tcp->flag&0x10)>>4,(tcp->flag&0x08)>>3,(tcp->flag&0x04)>>2,(tcp->flag&0x02)>>1,tcp->flag&0x01);
//    printf("窗口大小:\t%u\n",tcp->win);
//    printf("校验和:\t\t0x%016x\n",tcp->checksum);
//    printf("紧急指针:\t0x%016x\n",tcp->urp);
    printf("------------------\n");
    printf("********************************\n\n");


//    FILE *tcp_file = fopen("tcp.txt","a");
//    fprintf(tcp_file,"源端口:\t\t%u\n",tcp->src_port);
//    fprintf(tcp_file,"目的端口:\t%u\n",tcp->dst_port);
//    fprintf(tcp_file,"Seq:\t\t%u\n",tcp->seq);
//    fprintf(tcp_file,"Ack:\t\t%u\n",tcp->ack);
//    fprintf(tcp_file,"首部长度:\t%u 字节\n",((tcp->headlen&0xF0)>>4)*4);
//    fprintf(tcp_file,"标志位:\t\t%x%x%x%x%x%x\n",(tcp->flag&0x20)>>5,(tcp->flag&0x10)>>4,(tcp->flag&0x08)>>3,(tcp->flag&0x04)>>2,(tcp->flag&0x02)>>1,tcp->flag&0x01);
//    fprintf(tcp_file,"窗口大小:\t%u\n",tcp->win);
//    fprintf(tcp_file,"校验和:\t\t0x%016x\n",tcp->checksum);
//    fprintf(tcp_file,"紧急指针:\t0x%016x\n\n\n",tcp->urp);
//    fclose(tcp_file);

    FILE *fp = fopen("myCapture.txt","a");
    fprintf(fp,"源端口:\t\t%u\n",tcp->src_port);
    fprintf(fp,"目的端口:\t%u\n\n",tcp->dst_port);
    fclose(fp);

}

void save_udp(struct udp* udp)
{
//    u_short src_port;
//    u_short dst_port;
//    u_short len; //总长度
//    u_short checksum;

    printf("------UDP首部------\n");
    printf("源端口:\t\t%u\n",udp->src_port);
    printf("目的端口:\t%u\n",udp->dst_port);
//    printf("总长度:\t\t%u\n",udp->len);
//    printf("校验和:\t\t0x%016x\n",udp->checksum);
    printf("------------------\n");
    printf("********************************\n\n");

//    FILE *udp_file = fopen("udp.txt","a");
//    fprintf(udp_file,"源端口:\t\t%u\n",udp->src_port);
//    fprintf(udp_file,"目的端口:\t%u\n",udp->dst_port);
//    fprintf(udp_file,"总长度:\t\t%u\n",udp->len);
//    fprintf(udp_file,"校验和:\t\t0x%016x\n\n\n",udp->checksum);
//    fclose(udp_file);

    FILE *fp = fopen("myCapture.txt","a");
    fprintf(fp,"源端口:\t\t%u\n",udp->src_port);
    fprintf(fp,"目的端口:\t%u\n\n",udp->dst_port);
    fclose(fp);

}

void recvUDP(u_char* data,int data_len){
    printf("收到的数据为:%s\n",data);
    printf("数据长度:%d\n",data_len);
}

void process_packet(u_char *args, const struct pcap_pkthdr *header, const u_char *buffer) {
    pcap_dumper_t *dumper = (pcap_dumper_t *)args;
    pcap_dump((u_char*)dumper, header, buffer);
    struct ethernet *eth = (struct ethernet*)(buffer); //获取以太网头部
//    save_ethernet(eth);
    struct ip *iph = (struct ip*)(buffer + sizeof(struct ethernet));    // IP头
    save_ip(iph);
    if (iph->protocol == IPPROTO_TCP) {
        struct tcp *tcph = (struct tcp*)(buffer + sizeof(struct ethernet) + sizeof(struct ip));
        save_tcp(tcph);
    }
    else if(iph->protocol == IPPROTO_UDP){
        struct udp *udph = (struct udp*)(buffer + sizeof(struct ethernet) + sizeof(struct ip));
        save_udp(udph);
        u_char *data = (u_char*)(buffer + sizeof(struct ethernet) + sizeof(struct ip) + sizeof(struct udp));
        int data_len = ntohs(udph->len) - sizeof(struct udp);
        recvUDP(data,data_len);
    }


}