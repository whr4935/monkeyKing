#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <chrono>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <mutex>
#include <functional>
#include <memory>

#include <base/Log.h>

class Timer;
class Poll;
class Async;

using Callback = void(*)(Poll*, int, int);
using CbLoopExit = void(*)(void*);

struct Timepoint {
    std::function<void(Timer *)> cb;
    Timer *timer;
    std::chrono::system_clock::time_point timepoint;
    int nextDelay;
};

class Loop
{
public:
    class Object;

    Loop();
    virtual ~Loop();
    virtual void run();

    void addTimer(Timepoint t);
    void removeTimer(Timer* timer);
    std::chrono::system_clock::time_point now() {
        return mTimePoint = std::chrono::system_clock::now();
    }

    int add(Poll*, Callback cb, epoll_event *event);
    int remove(Poll*);
    int change(Poll*, Callback cb, epoll_event *newEvent);

    //called by other threads
    void sendRequest(std::function<void(void*)> cb, void *data);
    void requestExit();

private:
    void updateNextDelay();

    int mFd = -1;
    epoll_event mReadyEvents[1024];
    std::set<Object*> mObjects;
    std::chrono::system_clock::time_point mTimePoint;
    int mDelay = -1;
    std::vector<Timepoint> mTimers;
    bool mCancelledLastTimer = false;
    std::map<int,Callback> mCallbacks;

    std::recursive_mutex mRequestLock;
    std::list<std::pair<std::function<void(void*)>,void*>> mRequests;
    std::shared_ptr<Async> mHandler;
    bool mRequestExit = false;
};

class Loop::Object
{
public:
    Object(Loop* loop, void* userData, void (*cbLoopExit)(void* p))
        : mLoop(loop)
        , mUserData(userData)
        , mCbLoopExit(cbLoopExit)
    {
    }

    virtual ~Object() = 0;

    virtual void stop() {}
    virtual void onLoopExit() {
        if (mCbLoopExit)
            mCbLoopExit(mUserData);
        stop();
    }

    Loop* loop() const {return mLoop;}
    void* userData() const {return mUserData;}

protected:
    Loop* mLoop;
    void *mUserData;
    void (*mCbLoopExit)(void *p);
};

class Timer : public Loop::Object
{
public:
    Timer(Loop* loop, void* userData, CbLoopExit cbLoopExit = nullptr)
        : Object(loop, userData, cbLoopExit) { }
    virtual ~Timer() { }

    void start(std::function<void(Timer*)> cb, int timeout, int repeat = 0);
    void stop();   //stop() doesn't delete Timer object, we should do it ourself
};

class Poll : public Loop::Object
{
public:
    Poll(Loop *loop, int fd, void* userData, CbLoopExit cbLoopExit = nullptr)
        : Object(loop, userData, cbLoopExit)
        , mFd(fd) { }

    //caller should close fd himself
    virtual ~Poll() {
        if (mTimer) {
            cancelTimeout();
        }
    }

    void start(int events, Callback cb);
    void stop();
    void change(int events, Callback cb = nullptr);
    virtual void startTimeout(void (*cb)(Timer*), int timeout);
    void cancelTimeout();
    int fd() const { return mFd; }
    int events() const {return mEvents;}

protected:
    int mFd = -1;
    Timer* mTimer = nullptr;
    int mEvents = 0;
};

class Async : public Poll {
public:
    Async(Loop* loop, void* userData = nullptr, CbLoopExit cbLoopExit = nullptr)
        : Poll(loop, ::eventfd(0, EFD_CLOEXEC), userData, cbLoopExit) {
    }

    ~Async() {
        ::close(mFd);
    }

    void start(std::function<void(Async*)> cb);
    void notify();

private:
    std::function<void(Async*)> cb;
};

#endif // EPOLL_H
