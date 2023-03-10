#include <stdio.h>
#include <stdlib.h>     // 包含exit()
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>  //包含struct sockaddr_in
#include <ctype.h>      //包含toupper()
#include <strings.h>    //包含bzero()
#include <string.h>
#include <fcntl.h>

#define SERVER_IP "192.168.58.129"    // 服务器IP地址(在虚拟机中查看)
#define SERVER_PORT 10310     // 服务器端口号
#define BUFFER_SIZE 10240   //缓冲区大小

void showMenu(char* command)
{
    printf("********************\n");
    printf("欢迎使用文件传输系统！\n");
    printf("ls:查看服务器文件目录\n");
    printf("upload [filename]:上传指定文件\n");
    printf("download [filename]:下载指定文件\n");
    printf("********************\n");
    printf("请输入命令:");
    fgets(command,128,stdin);
}

int main()
{
    // 创建套接字
    int sock_fd = socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd<0)
    {
        printf("客户端套接字创建失败！\n");
        return -1;
    }

    // 连接到服务器
    struct sockaddr_in server;
    bzero(&server,sizeof(struct sockaddr_in));  // 初始化
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    printf("开始连接!\n");

    // 连接服务器
    int connected = connect(sock_fd,(struct sockaddr*)&server,sizeof(server));
    if(connected>=0)
    {
        printf("连接成功！\n");
        while (1)
        {
            char command[128]="";
            showMenu(command);  //用户输入命令
            // 将命令发送给服务器
            send(sock_fd,command, sizeof(command),0);

            // 根据用户输入命令不同，做不同的处理
            char* split_cmd = strtok(command," ");
            // ls命令
            if(strcmp(split_cmd,"ls\n")==0 || strcmp(split_cmd,"ls")==0)
            {
                char recv_buf[BUFFER_SIZE]="";
                // 接收服务器发来的文件列表，然后打印
                recv(sock_fd,recv_buf,BUFFER_SIZE,0);
                printf("\n------文件列表------\n");
                printf("%s",recv_buf);
                printf("------------------\n\n");
            }

                // download 命令
            else if(strcmp(split_cmd,"download")==0)
            {
                char* fileName = strtok(NULL," ");
                fileName[strlen(fileName)-1]='\0';
                char buffer[BUFFER_SIZE] = "";
                char filePath[128] = "/home/noname/Documents/netSec/lab1/cli/downloads/";
                strcat(filePath,fileName);
                printf("文件完整路径为:%s\n",filePath);
                FILE *fp = fopen(fileName, "wb");
                if (fp == NULL)
                {
                    printf("文件:\t%s 打开失败!\n", fileName);
                    exit(1);
                }

                // 从服务器端接收数据到buffer中
                bzero(buffer, BUFFER_SIZE);
                int length = 0;
                while(1)
                {
                    length = recv(sock_fd, buffer, BUFFER_SIZE, 0);
                    if (length < 0)
                    {
                        printf("数据接收失败!\n");
                        break;
                    }
                    if(strcmp(buffer,"Transfer Finished!")==0) break;
                    printf("buffer内容：%s\n",buffer);

                    int write_length = fwrite(buffer, sizeof(char),length,fp);
                    if (write_length < length)
                    {
                        printf("File:\t%s Write Failed!\n", fileName);
                        break;
                    }
                    bzero(buffer, BUFFER_SIZE);
                }

                printf("文件\t %s 传输完成！!\n", fileName);

                // 传输完毕，关闭文件指针
                fclose(fp);
            }

                // upload 命令
            else if(strcmp(split_cmd,"upload")==0)
            {
                char* fileName = strtok(NULL," ");
                fileName[strlen(fileName)-1]='\0';
                char filePath[128] = "/home/noname/Documents/netSec/lab1/cli/uploads/";
                strcat(filePath,fileName);
                printf("文件完整路径为:%s\n",filePath);
                FILE *fp = fopen(filePath, "rb");
                if (fp == NULL)
                {
                    perror("打开文件失败啦");
                }

                else
                {
                    char buffer[BUFFER_SIZE]="";
                    bzero(buffer, BUFFER_SIZE);
                    int file_block_length = 0;
                    while( (file_block_length = fread(buffer, sizeof(char), BUFFER_SIZE, fp)) > 0)
                    {
                        printf("file_block_length = %d\n", file_block_length);

                        // 发送buffer中的字符串到new_server_socket,实际上就是发送给客户端
                        printf("发送buffer：%s\n",buffer);
                        if (send(sock_fd, buffer, file_block_length, 0) < 0)
                        {
                            printf("Send File:\t%s Failed!\n", fileName);
                            break;
                        }
                        bzero(buffer, BUFFER_SIZE);
                    }
                    fclose(fp);
                    sleep(5);
                    send(sock_fd,"Transfer Finished!",32,0);
                    printf("File:\t%s Transfer Finished!\n", fileName);
                }

            }

                // disconnect命令
            else if(strcmp(split_cmd,"disconnect")==0 || strcmp(split_cmd,"disconnect\n")==0 )
            {
                printf("正在断开连接...\n");
                break;
            }

                // 其他指令可能是打错了，打印提示信息
            else
            {
                printf("指令%s存在错误！\n",command);
                continue;
            }
        }
        printf("连接已断开\n");
        close(sock_fd);
        return 0;
    }

    printf("连接失败\n");
    return 0;
}