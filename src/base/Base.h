#ifndef _SILENT_DREAM_H
#define _SILENT_DREAM_H

#include <set>
#include "Socket.h"

class Base  : public SocketServerHandler
{
public:
    Base();
    virtual ~Base();

    virtual int init();
    virtual int destroy();

    virtual void onAccepted(int sockFd, struct sockaddr_in* addr, socklen_t addrLen);
    virtual void onError(Socket::ErrorCode);

    //void onWorkerFinished(SilentDreamWorker* worker);

private:
    int daemonize();
    int checkRunning(); 
    int initServer();

    Socket* mSocket = nullptr;
    //std::set<SilentDreamWorker*> mWorkers;
    DISALLOW_EVIL_CONSTRUCTORS(Base);
};


#endif
