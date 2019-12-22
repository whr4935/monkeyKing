#include <assert.h>
#include "Epoll.h"

Loop::Object::~Object() {}

Loop::Loop()
{
    mFd = epoll_create(1024);
    mTimePoint = std::chrono::system_clock::now();
    mHandler.reset(new Async(this));

    mHandler->start([this](Async* ) {
        std::lock_guard<std::recursive_mutex> _l(mRequestLock);

        while (mRequests.size()) {
            auto it = mRequests.begin();
            it->first(it->second);
            mRequests.erase(it);
        }
    });
}

Loop::~Loop()
{
   ::close(mFd);
}

void Loop::run()
{
    mTimePoint = std::chrono::system_clock::now();

    for (;;) {
        if (mObjects.size() == 0) {
//            LOGI("Epoll loop exited!");
            break;
        }

        int numFdReady = epoll_wait(mFd, mReadyEvents, 1024, mDelay);
        mTimePoint = std::chrono::system_clock::now();

        for (int i = 0; i < numFdReady; ++i) {
            Poll* poll = (Poll*)mReadyEvents[i].data.ptr;
            int status = -bool(mReadyEvents[i].events & (EPOLLERR|EPOLLHUP));
            auto it = mCallbacks.find(poll->fd());
            if (it != mCallbacks.end()) {
                it->second(poll, status, mReadyEvents[i].events);
            } else {
                LOGE("no callback!");
            }
        }

        while (mTimers.size() && mTimers[0].timepoint < mTimePoint) {
            Timer* timer = mTimers[0].timer;
            mCancelledLastTimer = false;
            mTimers[0].cb(mTimers[0].timer);

            //timer stoped in callback function
            if (mCancelledLastTimer)
                continue;

            int repeat = mTimers[0].nextDelay;
            auto cb = mTimers[0].cb;
            mTimers.erase(mTimers.begin());
            if (repeat) {
                timer->start(cb, repeat, repeat);
            }
        }
    }
}

void Loop::addTimer(Timepoint t)
{
    mTimers.insert( std::upper_bound(mTimers.begin(), mTimers.end(), t, [](const Timepoint&a, const Timepoint&b) {
        return a.timepoint < b.timepoint;
    }), t );

    mObjects.insert(t.timer);
    updateNextDelay();
}

void Loop::removeTimer(Timer *timer)
{
    auto pos = mTimers.begin();
    for (Timepoint &t : mTimers) {
        if (t.timer == timer) {
            mTimers.erase(pos);
            break;
        }
        ++pos;
    }

    auto it = mObjects.find(timer);
    if (it != mObjects.end()) {
        mObjects.erase(it);
    }

    mCancelledLastTimer = true;
    updateNextDelay();
}

void Loop::updateNextDelay() {
    mDelay = -1;
    if (mTimers.size()) {
        mDelay = std::max<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                       mTimers[0].timepoint - mTimePoint).count(), -1);
    }
}

int Loop::add(Poll* p, Callback cb, epoll_event *event) {
    int ret = epoll_ctl(mFd, EPOLL_CTL_ADD, p->fd(), event);
    if (ret == 0) {
        mCallbacks[p->fd()] = cb;

        mObjects.insert(p);
    }
    assert(ret == 0);

    return ret;
}

int Loop::remove(Poll* p) {
    epoll_event e;

    int ret = epoll_ctl(mFd, EPOLL_CTL_DEL, p->fd(), &e);
    if (ret == 0) {
        mCallbacks[p->fd()] = nullptr;

        auto it = mObjects.find(p);
        if (it != mObjects.end()) {
            mObjects.erase(it);
        }
    }
//    assert(ret == 0);

    return ret;
}

int Loop::change(Poll* p, Callback cb, epoll_event *newEvent) {
    int ret = epoll_ctl(mFd, EPOLL_CTL_MOD, p->fd(), newEvent);
    if (ret == 0) {
        mCallbacks[p->fd()] = cb;
    }

    return ret;
}

void Loop::sendRequest(std::function<void(void *)> cb, void *data)
{
    std::lock_guard<std::recursive_mutex> _l(mRequestLock);

    if (mRequestExit)
        return;

    mRequests.push_back({cb, data});
    mHandler->notify();
}

void Loop::requestExit()
{
    std::lock_guard<std::recursive_mutex> _l(mRequestLock);

//    LOGI("Epoll loop request exit...");
    mRequests.clear();
    auto cb = [this](void *) {
        //check whether iterator invalid !!!
       for (auto p : mObjects) {
           p->onLoopExit();
       }
    };
    mRequests.push_back({cb, this});
    mRequestExit = true;
    mHandler->notify();
}

//////////////////////////////////
void Timer::start(std::function<void (Timer *)> cb, int timeout, int repeat)
{
    std::chrono::system_clock::time_point timepoint = mLoop->now() +
                    std::chrono::milliseconds(timeout);
    Timepoint t = {cb, this, timepoint, repeat};
    mLoop->addTimer(t);
}

void Timer::stop()
{
//    LOGV("stop timer:%p", this);
    mLoop->removeTimer(this);
}

//////////////////////////////////
void Poll::start(int events, Callback cb)
{
    epoll_event event;
    event.events = events;
    event.data.ptr = this;
    mLoop->add(this, cb, &event);

    mEvents = events;
}

void Poll::stop()
{
//    LOGV("stop poll:%p", this);
    mLoop->remove(this);
}

void Poll::change(int events, Callback cb)
{
    epoll_event event;
    event.events = events;
    event.data.ptr = this;
    mLoop->change(this, cb, &event);

    mEvents = events;
}

void Poll::startTimeout(void (*cb)(Timer *), int timeout)
{
    mTimer = new Timer(mLoop, this);
    mTimer->start(cb, timeout);
}

void Poll::cancelTimeout()
{
    mTimer->stop();
    delete mTimer;
    mTimer = nullptr;
}

//////////////////////////////////
void Async::start(std::function<void (Async *)> cb)
{
    this->cb = cb;

    Poll::start(EPOLLIN, [](Poll* p, int, int) {
        uint64_t val;
        if (::read(p->fd(), &val, 8) == 8) {
            ((Async*)p)->cb((Async*)p);
        }
    });
}

void Async::notify()
{
    int64_t one = 1;
    if (::write(mFd, &one, 8) != 8) {
        LOGE("Async::notify failed!");
        return;
    }
}

