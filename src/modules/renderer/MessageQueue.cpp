#include "MessageQueue.h"
#include <android/log.h>

#define LOG_NDEBUG 0
#define LOG_TAG "MessageQueue"

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, ##__VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, ##__VA_ARGS__)
#define ALOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ##__VA_ARGS__)

class MessageQueue::Fence
{
public:
    Fence()
        : mDone(false)
    {
        pthread_mutex_init(&mLock, NULL);
        pthread_cond_init(&mCond, NULL);
    }

    ~Fence()
    {
        pthread_mutex_destroy(&mLock);
        pthread_cond_destroy(&mCond);
    }

    void wait()
    {
        pthread_mutex_lock(&mLock);
        while (!mDone) {
            pthread_cond_wait(&mCond, &mLock);
        }
        pthread_mutex_unlock(&mLock);
    }

    void signal()
    {
        pthread_mutex_lock(&mLock);
        mDone = true;
        pthread_mutex_unlock(&mLock);
        pthread_cond_signal(&mCond);
    }

private:
    pthread_mutex_t mLock;
    pthread_cond_t mCond;
    bool mDone;
};

///////////////////////////////////
MessageQueue::Message::Message(int _what, int _ext1, int _ext2)
    : what(_what), ext1(_ext1), ext2(_ext2)
    , mFence(NULL)
    , ret(0)
{
}

MessageQueue::Message::~Message()
{
    if (mFence) {
        delete mFence;
        mFence = NULL;
    }
}

void MessageQueue::Message::setFence()
{
    mFence = new Fence();
}

bool MessageQueue::Message::hasFence() const
{
    return mFence != NULL;
}

void MessageQueue::Message::wait() const
{
    mFence->wait();
}

void MessageQueue::Message::signal() const
{
    mFence->signal();
}

/////////////////////////////
MessageQueue::MessageQueue()
{
    pthread_mutex_init(&mMutex, NULL);
    pthread_cond_init(&mCond, NULL);
}

MessageQueue::~MessageQueue()
{
    pthread_mutex_destroy(&mMutex);
    pthread_cond_destroy(&mCond);
}

void MessageQueue::postMessage(Message* msg)
{
    pthread_mutex_lock(&mMutex);
    mMessageQueue.push_back(msg);
    pthread_mutex_unlock(&mMutex);
    pthread_cond_signal(&mCond);
}

int MessageQueue::pushMessage(Message* msg)
{
    msg->setFence();
    postMessage(msg);
    msg->wait();

    int ret = msg->ret;
    delete msg;

    return ret;
}

MessageQueue::Message* MessageQueue::getMessage()
{
    pthread_mutex_lock(&mMutex);
    while (mMessageQueue.empty()) {
        pthread_cond_wait(&mCond, &mMutex);
    }
    Message* msg = mMessageQueue.front();
    mMessageQueue.pop_front();
    pthread_mutex_unlock(&mMutex);

    return msg;
}
