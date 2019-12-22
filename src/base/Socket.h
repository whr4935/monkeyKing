#ifndef SOCKET_H
#define SOCKET_H

#include <sys/socket.h>
#include <netinet/in.h>

#include <base/Log.h>
#include <string>
#include "Epoll.h"

class SocketClientHandler;
class SocketServerHandler;

class Socket
{
public:
    enum {
        CLIENT,
        SERVER,
    };

    enum ErrorCode {
        ERROR_UNKNOWN = 0x1000,
        ERROR_CONNECT,
        ERROR_RECV,
        ERROR_SEND,
    };

    Socket(Loop* loop);
    ~Socket();

    int initAddress(std::string address);
    int createSocket();
    int initServer();
    int connect();
    int iniSocket(Poll*poll, struct sockaddr_in* addr, socklen_t addrLen);
    int sockFd() const {
        return mSockFd;
    }

    void setClientHandler(SocketClientHandler* clientHandler);
    void setServerHandler(SocketServerHandler* serverHandler);

    ssize_t write(const void* buf, size_t len);

    static void cbAccept(Poll* p, int status, int event);
    static void cbConnect(Poll* p, int status, int event);
    template <bool isServer>
    static void ioHandler(Poll* p, int status, int event);

private:
    int onAccept();

private:
    int mDomain = AF_INET;
    int mType = SOCK_STREAM;
    int mProtocol = IPPROTO_TCP;
    struct sockaddr_in mSockAddr;
    socklen_t mSockAddrLen = sizeof(struct sockaddr_in);

    int mSockFd = -1;
    Poll* mPoll = nullptr;
    Loop* mLoop = nullptr;

    SocketClientHandler* mClientHandler = nullptr;
    SocketServerHandler* mServerHandler = nullptr;
};

class SocketBaseHandler
{
public:
    virtual ~SocketBaseHandler() = 0;
    virtual void onData(const void*, size_t) {}
    virtual void onDisConnected() {}
    virtual void onError(Socket::ErrorCode) = 0;
};

class SocketClientHandler : public SocketBaseHandler
{
public:
    virtual ~SocketClientHandler() = 0;
    virtual void onConnected() {}
};

class SocketServerHandler : public SocketBaseHandler
{
public:
    virtual ~SocketServerHandler() = 0;
    virtual void onAccepted(int , struct sockaddr_in* , socklen_t ) {}
};

#endif // SOCKET_H
