#ifndef SIGNALHANDLER_H
#define SIGNALHANDLER_H

#include <pthread.h>
#include <signal.h>
#include <ostream>
#include <functional>

#include "Epoll.h"


class SignalHandler
{
public:
    static SignalHandler& instace();
    ~SignalHandler();

    void setCustomSignalHandler(sigset_t set, std::function<void(int)> handler);
    int install(Loop *loop);

private:
    SignalHandler();
    static SignalHandler* self;

    int initPlatformSignalHandlers();
    static void unexpectedSignalHandler(int, siginfo_t*, void*);
    static const char* getSignalName(int signal_number);
    static const char* getSignalCodeName(int signal_number, int signal_code);
    static void dumpstack(std::ostream &os);

    int initCustomSignalHandlers();
    static void* customSignalThreadFn(void *arg);


    Loop* mLoop = nullptr;
    sigset_t mCustomSignalMask;
    std::function<void(int)> mCustomSignalHandler;

    sigset_t mSignalMask;
    sigset_t mOldSignalMask;
    pthread_t mSignalTid;
};

#endif // SIGNALHANDLER_H
