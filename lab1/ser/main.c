#include <stdio.h>
#include <stdlib.h>     // 包含exit()
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>  //包含struct sockaddr_in
#include <strings.h>    //包含bzero()
#include <dirent.h>
#include <string.h>


#define SERVER_PORT 10310     // 服务器端口号
#define BUFFER_SIZE 10240   //缓冲区大小

/**
 * 读取目录下的文件列表
 * @param basePath 路径
 * @param fileList 存储文件列表的字符串
 * @return 成功状态
 */
int readFileList(char *basePath,char* fileList)
{
    DIR *dir;
    struct dirent *ptr;
    char base[1000];

    if ((dir=opendir(basePath)) == NULL)
    {
        perror("Open dir error...");
        exit(1);
    }

    while ((ptr=readdir(dir)) != NULL)
    {
        if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0)    ///current dir OR parrent dir
            continue;
        else if(ptr->d_type == 8)    ///file
        {
//            printf("文件名：:%s\n",ptr->d_name);
            strcat(fileList,ptr->d_name);
            strcat(fileList,"\n");
        }
        else if(ptr->d_type == 10)    ///link file
        {
//            printf("文件名:%s\n", ptr->d_name);
            strcat(fileList,ptr->d_name);
            strcat(fileList,"\n");
        }
        else if(ptr->d_type == 4)    ///dir
        {
            memset(base,'\0',sizeof(base));
            strcpy(base,basePath);
            strcat(base,"/");
            strcat(base,ptr->d_name);
            readFileList(base,fileList);
        }
    }
    closedir(dir);
    return 1;
}


int main()
{
    int sock_fd = socket(AF_INET,SOCK_STREAM,0); // 使用IPv4 和 TCP

    if(sock_fd<0)
    {
        printf("服务器套接字创建失败！\n");
        return -1;
    }

    struct sockaddr_in server;
    bzero(&server,sizeof(struct sockaddr_in));  // 初始化

    // 设置远程服务器的信息
//    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    bind(sock_fd,(struct sockaddr*)&server,sizeof(server));

    // 监听
    listen(sock_fd,5);
    printf("正在监听端口[%d]...\n",SERVER_PORT);

    // 等待客户端连接
    int connect_fd = accept(sock_fd,NULL,NULL);
    if(connect_fd>=0)
    {
        printf("连接成功!\n");

        while (1)
        {
            char buf[BUFFER_SIZE] ="";
            recv(connect_fd,buf,BUFFER_SIZE,0);
            printf("服务器收到：%s\n",buf);
            char* split_cmd = strtok(buf," ");  // 切分命令

            // 查看目录和文件
            if(strcmp(split_cmd,"ls\n")==0 || strcmp(split_cmd,"ls")==0)
            {
                char fileList[BUFFER_SIZE]="";
                readFileList("/home/noname/Documents/netSec/lab1/ser/files",fileList);
                send(connect_fd,fileList,BUFFER_SIZE,0);
            }

                // 下载文件
            else if(strcmp(split_cmd,"download")==0)
            {
                printf("进入下载\n");
                char* fileName = strtok(NULL,"");
                fileName[strlen(fileName)-1]='\0';
                printf("文件名为%s\n",fileName);
                char filePath[128] = "/home/noname/Documents/netSec/lab1/ser/files/";
                strcat(filePath,fileName);
                printf("文件完整路径为:%s\n",filePath);
                FILE* fp = fopen(filePath,"rb");
                if(fp==NULL)
                {
                    printf("%s文件打开失败!\n",fileName);
                    continue;
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
                        if (send(connect_fd, buffer, file_block_length, 0) < 0)
                        {
                            printf("Send File:\t%s Failed!\n", fileName);
                            break;
                        }
                        bzero(buffer, BUFFER_SIZE);
                    }
                    fclose(fp);
                    sleep(5);
                    send(connect_fd,"Transfer Finished!",32,0);
                    printf("File:\t%s Transfer Finished!\n", fileName);
                }
            }

                // 上传文件
            else if(strcmp(split_cmd,"upload")==0)
            {
                char* fileName = strtok(NULL," ");
                fileName[strlen(fileName)-1]='\0';
                char buffer[BUFFER_SIZE] = "";
                char filePath[128] = "/home/noname/Documents/netSec/lab1/ser/files/downloads/";
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
                    length = recv(connect_fd, buffer, BUFFER_SIZE, 0);
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

                // 传输完毕，关闭socket
                fclose(fp);
            }

                // 断开连接
            else if(strcmp(split_cmd,"disconnect\n")==0 || strcmp(split_cmd,"disconnect")==0)
            {
                printf("正在断开连接...\n");
                break;
            }
        }
        close(sock_fd);
        printf("连接已断开\n");
    }
}
