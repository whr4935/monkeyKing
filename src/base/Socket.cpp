#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "Socket.h"

#define SERV_PORT  28979

Socket::Socket(Loop* loop)
    : mLoop(loop)
{
    memset(&mSockAddr, 0, sizeof(struct sockaddr_in));
}

Socket::~Socket()
{
    mPoll->stop();
    delete mPoll;
    ::shutdown(mSockFd, SHUT_RDWR);
    ::close(mSockFd);
}

int Socket::initAddress(std::string serverAddr)
{
    struct addrinfo *addr;
    struct addrinfo hint;

    memset(&hint, 0, sizeof(hint));

    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_protocol = 0;
    int ret = getaddrinfo(serverAddr.c_str(), nullptr, &hint, &addr);
    if (ret != 0) {
        LOGE("serverAddr:%s, getaddrinfo failed:%s",serverAddr.c_str(), gai_strerror(ret));
        return -1 ;
    }

    std::shared_ptr<addrinfo> _(addr, [](struct addrinfo* p) {
        ::freeaddrinfo(p);
    });

    struct sockaddr_in* addr_in = (struct sockaddr_in*)addr->ai_addr;
    addr_in->sin_port = htons(SERV_PORT);
    mSockAddr = *addr_in;
    mSockAddrLen = addr->ai_addrlen;
    mDomain = AF_INET;
    mType = SOCK_STREAM;
    mProtocol = IPPROTO_TCP;

    return 0;
}

int Socket::createSocket()
{
    mSockFd = ::socket(mDomain, mType|SOCK_NONBLOCK, mProtocol);
    if (mSockFd < 0) {
        LOGE("create socket failed:%s", strerror(errno));
        return -1;
    }

    int reuse = 1;
    if (setsockopt(mSockFd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
        LOGE("setsockopt failed!");
        return -1;
    }

    mPoll = new Poll(mLoop, mSockFd, this);

    return 0;
}

int Socket::initServer()
{
    LOGI("server:%s:%d", inet_ntoa(mSockAddr.sin_addr), ntohs(mSockAddr.sin_port));

    int err = ::bind(mSockFd, (struct sockaddr*)&mSockAddr, sizeof(struct sockaddr));
    if (err < 0) {
        LOGE("bind:%s", strerror(errno));
        return -1;
    }

    err = ::listen(mSockFd, 10);
    if (err < 0) {
        LOGE("listen:%s", strerror(errno));
        return -1;
    }

    mPoll->start(EPOLLIN, cbAccept);

    return 0;
}

int Socket::connect()
{
    LOGI("connect to:%s:%d", inet_ntoa(mSockAddr.sin_addr), ntohs(mSockAddr.sin_port));

    int err = ::connect(mSockFd, (struct sockaddr*)&mSockAddr, mSockAddrLen);
    if (err == 0) {
        LOGI("connect success!");
    } else {
        if (errno != EINPROGRESS) {
            LOGI("connect failed:%s", strerror(errno));
            return -1;
        }
        mPoll->start(EPOLLOUT, cbConnect);
    }

    return 0;
}

int Socket::iniSocket(Poll *poll, struct sockaddr_in *addr, socklen_t addrLen)
{
    mDomain = addr->sin_family;
    mType = SOCK_STREAM;
    mProtocol = IPPROTO_TCP;
    mSockAddr = *addr;
    mSockAddrLen = addrLen;
    mPoll = poll;
    mSockFd = poll->fd();

    return 0;
}

void Socket::setClientHandler(SocketClientHandler *clientHandler)
{
    mClientHandler = clientHandler;
}

void Socket::setServerHandler(SocketServerHandler *serverHandler)
{
    mServerHandler = serverHandler;
}

ssize_t Socket::write(const void *buf, size_t len)
{
    ssize_t ret = 0;

    ret = ::write(mSockFd, buf, len);

    return ret;
}

////////////////////////////////////////////////////////
void Socket::cbAccept(Poll *p, int status, int event)
{
    Socket* s = static_cast<Socket*>(p->userData());

    if (status < 0) {
        event |= EPOLLIN;
    }

    if (event & EPOLLIN) {
        s->onAccept();
    }
}

int Socket::onAccept()
{
    int clientSock;
    struct sockaddr client;
    socklen_t sockLen = sizeof(struct sockaddr);
    clientSock = ::accept(mSockFd, &client, &sockLen);
    if (clientSock < 0) {
        assert (errno!=EAGAIN && errno!=EWOULDBLOCK);
        LOGE("accept failed:%s", strerror(errno));
        return -1;
    }

    struct sockaddr_in* client_in = (struct sockaddr_in*)&client;

    if (mServerHandler) {
        mServerHandler->onAccepted(clientSock, client_in, sockLen);
    }

    return 0;
}

void Socket::cbConnect(Poll *p, int status, int event)
{
    Socket* s = static_cast<Socket*>(p->userData());

    if (status < 0) {
        event |= EPOLLOUT;
    }

    int ret;
    int err;
    bool fail = false;
    socklen_t len = sizeof(err);
    if (event & EPOLLOUT) {
        ret = getsockopt(p->fd(), SOL_SOCKET, SO_ERROR, &err, &len);
        if (ret < 0) {
            LOGE("getsockopt failed:%s", strerror(errno));
            fail = true;
        } else {
            if (err != 0) {
                LOGE("cbConnect failed:%s", strerror(err));
                fail = true;
            } else {
                LOGI("cbConnect success!");
            }
        }

        if (!fail) {
            p->change(EPOLLIN, ioHandler<CLIENT>);
            s->mClientHandler->onConnected();
        } else {
            p->stop();
            s->mClientHandler->onError(Socket::ERROR_CONNECT);
        }
    }
}

template <bool isServer>
void Socket::ioHandler(Poll *p, int status, int event)
{
    Socket* s = static_cast<Socket*>(p->userData());
    bool fail = false;
    int len;
    uint8_t recvBuf[1024];

    SocketBaseHandler* handler;
    if (isServer) {
        handler = s->mServerHandler;
    } else {
        handler = s->mClientHandler;
    }

    if (status < 0) {
        event |= EPOLLIN|EPOLLOUT;
    }

    if (event & EPOLLIN) {
        memset(recvBuf, 0, sizeof(recvBuf));
        len = ::recv(p->fd(), recvBuf, sizeof(recvBuf), 0);
        if (len <= 0) {
            p->change(p->events()&~EPOLLIN);
            if (len == 0) {
                handler->onDisConnected();
                return;
            }

            fail = true;
            LOGE("cbServer error:%s", strerror(errno));
        }

        if (!fail) {
            handler->onData(recvBuf, len);
        } else {
            handler->onError(ERROR_RECV);
        }
    }

    if (event & EPOLLOUT) {

    }
}

template void Socket::ioHandler<Socket::SERVER>(Poll*, int, int);
template void Socket::ioHandler<Socket::CLIENT>(Poll*, int, int);

SocketBaseHandler::~SocketBaseHandler() {}
SocketClientHandler::~SocketClientHandler() {}
SocketServerHandler::~SocketServerHandler() {}
