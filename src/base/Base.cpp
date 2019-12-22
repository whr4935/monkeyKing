#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <execinfo.h>
#include <assert.h>
#include <arpa/inet.h>

#include <base/Log.h>
#include <base/PluginManager.h>
#include "Base.h"
#include "ArgumentParser.h"
#include "Epoll.h"
#include "Socket.h"


Base::Base()
{
}

Base::~Base()
{
    //for (auto &p : mWorkers) {
        //delete p;
    //}

    if (mSocket != nullptr) {
        delete mSocket;
        mSocket = nullptr;
    }
}

int Base::init()
{
    if (daemonize() < 0) {
        return -1;
    }

    if (checkRunning() != 0) {
        return -1;
    }

    //SilentDreamBase::init();
    LOGI("silentdream daemon start...");
    LOGI("RootDir:%s", getRootDir());

    PluginManager::instance()->loadPlugins();

    if (initServer() < 0) {
        return -1;
    }

    return 0;
}

int Base::destroy()
{
    //const char* pid_file = "run/silentdream.pid";

    //if (::access(pid_file, F_OK) == 0) {
        //if (::unlink(pid_file) < 0) {
            //LOGW("delete pid file:%s failed:%s", pid_file, strerror(errno));
        //}
    //}

    return 0;
}

void Base::onAccepted(int sockFd, struct sockaddr_in *addr, socklen_t addrLen)
{
#if 0
    LOGI("client [%s:%d] connected!", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));

    if (fcntl(sockFd, F_SETFL, fcntl(sockFd, F_GETFL) | O_NONBLOCK) < 0) {
        LOGW("set client socket nonblock failed!", strerror(errno));
    }

    Socket* s = new Socket(mLoop);
    Poll* poll = new Poll(mLoop, sockFd, s);
    s->iniSocket(poll, addr, addrLen);

    SilentDreamWorker* worker = new SilentDreamWorker(mLoop, s, this);
    s->setServerHandler(worker);

    mWorkers.insert(worker);
    poll->start(EPOLLIN, Socket::ioHandler<Socket::SERVER>);
#endif
}

void Base::onError(Socket::ErrorCode err)
{
//    LOGE("onError:%#x", err);
//    mLoop->requestExit();
}

//void Base::onWorkerFinished(SilentDreamWorker *worker)
//{
    //auto it = mWorkers.find(worker);
    //assert(it != mWorkers.end());

    //delete worker;
    //mWorkers.erase(worker);
//}

//////////////////////////////////////////////
int Base::daemonize()
{
//    umask(0);

    int pid;
    if ((pid = fork()) < 0) {
        LOGE("fork:%s", strerror(errno));
        return -1;
    } else if (pid != 0) {
        exit(0);
    }

    if (setsid() < 0) {
        LOGE("setsid failed:%s",strerror(errno));
        return -1;
    }

    if ((pid = fork()) < 0) {
        LOGE("fork:%s", strerror(errno));
        return -1;
    } else if (pid != 0) {
        exit(0);
    }

    return 0;
}

int Base::checkRunning()
{
    int fd;

    if (ensureDirectoryExist("run") < 0) {
        return -1;
    }

    if ((fd = open("run/silentdream.pid", O_RDWR|O_CREAT, 0644)) < 0) {
        LOGE("create pid file failed:%s", strerror(errno));
        return -1;
    }

    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    if (fcntl(fd, F_SETLK, &fl) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            close(fd);
            LOGW("already running!");
            return 1;
        }

        LOGE("lockfile failed:%s", strerror(errno));
        return -1;
    }

    ftruncate(fd, 0);

    char buf[16];
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf)+1);

    return 0;
}

int Base::initServer()
{
    //mSocket = new Socket(mLoop);
    //mSocket->setServerHandler(this);
    //if (mSocket->initAddress("0.0.0.0") < 0) {
        //return -1;
    //}
    //if (mSocket->createSocket() < 0) {
        //return -1;
    //}
    //if (mSocket->initServer() < 0) {
        //return -1;
    //}

    return 0;
}





