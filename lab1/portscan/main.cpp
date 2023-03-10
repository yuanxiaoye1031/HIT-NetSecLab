#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QLabel>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QListWidget>

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#include <thread>
#include <vector>
#include <mutex>

std::mutex resultMutex;
std::vector<std::pair<QString, int>> results;


void attemptConnect(const char* ip,unsigned short port,int timeout)
{
    //初始化
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR) {
        printf("WSAStartup function failed\n");
        return;
    }

    SOCKET connectSocket= INVALID_SOCKET;
    do {
        //创建socket
        connectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connectSocket == INVALID_SOCKET) {
            printf("socket function failed with error: %ld\n", WSAGetLastError());
            break;
        }

        //socket设置为非阻塞
        unsigned long on = 1;
        if (ioctlsocket(connectSocket, FIONBIO, &on) < 0) {
            printf("ioctlsocket failed\n");
            break;
        }

        //尝试连接
        sockaddr_in clientService;
        clientService.sin_family = AF_INET;
        clientService.sin_addr.s_addr = inet_addr(ip);
        clientService.sin_port = htons(port);

        int ret = connect(connectSocket, (struct sockaddr*)&clientService, sizeof(clientService));
        if (ret == 0) {
            printf("connect success1\n");
            return;
        }

        //因为是非阻塞的，这个时候错误码应该是WSAEWOULDBLOCK，Linux下是EINPROGRESS
        if (ret < 0 && WSAGetLastError() != WSAEWOULDBLOCK) {
            printf("connect failed with error: %ld\n", WSAGetLastError());
            return;
        }


        fd_set writeset;
        FD_ZERO(&writeset);
        FD_SET(connectSocket, &writeset);
        timeval tv;
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        ret = select(connectSocket + 1, NULL, &writeset, NULL, &tv);
        if (ret == 0) {
            printf("connect timeout\n");
        } else if (ret < 0) {
            printf("connect failed with error: %ld\n", WSAGetLastError());
        } else {
            printf("connect success2\n");
        }
    } while (false);
    if (connectSocket != INVALID_SOCKET) {
        closesocket(connectSocket);
    }
    WSACleanup();
}

void scanPorts(QString ip, int startPort, int endPort)
{
    WSADATA wsaData;
    int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (err != 0)
    {
        return;
    }

    SOCKET sock;
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        WSACleanup();
        return;
    }

    sockaddr_in sockAddr;
    memset(&sockAddr, 0, sizeof(sockaddr_in));
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_addr.s_addr = inet_addr(ip.toStdString().c_str());



    for (int i = startPort; i <= endPort; i++)
    {
        sockAddr.sin_port = htons(i);
        if (connect(sock, (SOCKADDR*)&sockAddr, sizeof(sockAddr)) != SOCKET_ERROR)
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            results.push_back(std::make_pair(ip, i));
            closesocket(sock);
            break;
        }
    }
    closesocket(sock);
    WSACleanup();
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QLabel* ipLabel_start = new QLabel("Satrt IP:");
    QLineEdit* ipEdit_start = new QLineEdit("127.0.0.1");

    QLabel* ipLabel_end = new QLabel("End IP:");
    QLineEdit* ipEdit_end = new QLineEdit("127.0.0.1");

    QLabel* portLabel = new QLabel("Port Range:");
    QSpinBox* startPortSpin = new QSpinBox;
    startPortSpin->setRange(1, 65535);
    startPortSpin->setValue(1);
    QSpinBox* endPortSpin = new QSpinBox;
    endPortSpin->setRange(1, 65535);
    endPortSpin->setValue(10000);

    QLabel* threadLabel = new QLabel("Threads:");
    QSpinBox* threadSpin = new QSpinBox;
    threadSpin->setRange(1, 100);
    threadSpin->setValue(10);

    QPushButton* scanBtn = new QPushButton("Scan");
    QListWidget* resultWidget = new QListWidget;

    QVBoxLayout* layout = new QVBoxLayout;
    QHBoxLayout* ipLayout = new QHBoxLayout;
    ipLayout->addWidget(ipLabel_start);
    ipLayout->addWidget(ipEdit_start);
    ipLayout->addWidget(ipLabel_end);
    ipLayout->addWidget(ipEdit_end);
    layout->addLayout(ipLayout);

    QHBoxLayout* portLayout = new QHBoxLayout;
    portLayout->addWidget(portLabel);
    portLayout->addWidget(startPortSpin);
    portLayout->addWidget(new QLabel("-"));
    portLayout->addWidget(endPortSpin);
    layout->addLayout(portLayout);

    QHBoxLayout* threadLayout = new QHBoxLayout;
    threadLayout->addWidget(threadLabel);
    threadLayout->addWidget(threadSpin);
    layout->addLayout(threadLayout);

    layout->addWidget(scanBtn);
    layout->addWidget(resultWidget);

    QWidget* widget = new QWidget;
    widget->setLayout(layout);
    widget->show();

    QObject::connect(scanBtn, &QPushButton::clicked, [&](){
        resultWidget->clear();
        results.clear();

        QString ip = ipEdit_start->text();
        int startPort = startPortSpin->value();
        int endPort = endPortSpin->value();
        int threads = threadSpin->value();

        if (endPort < startPort)
        {
            std::swap(startPort, endPort);
        }

        std::vector<std::thread> threadList;
        int portPerThread = (endPort - startPort + 1) / threads;
        for (int i = 0; i < threads; i++)
        {
            int threadStartPort = startPort + i * portPerThread;
            int threadEndPort = min(endPort, startPort + (i + 1) * portPerThread - 1);
            threadList.push_back(std::thread(scanPorts, ip, threadStartPort, threadEndPort));
        }

        for (auto& thread : threadList)
        {
            thread.join();
        }

        for (const auto& result : results)
        {
            QString itemText = QString("%1:%2").arg(result.first).arg(result.second);
            resultWidget->addItem(itemText);
        }
    });

    return app.exec();
}


