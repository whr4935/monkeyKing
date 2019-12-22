#ifndef _MESSAGE_QUEUE_H
#define _MESSAGE_QUEUE_H

#include <pthread.h>
#include <list>

class MessageQueue {
public:
    class Fence;

    struct Message {
        int what;
        int ext1;
        int ext2;
        int ret;

        explicit Message(int _what = 0, int _ext1=0, int _ext2=0);
        ~Message();
        void setFence();
        bool hasFence() const;
        void wait() const;
        void signal() const;

    private:
        Fence* mFence;
    };

public:
    MessageQueue();
    ~MessageQueue();

    void postMessage(Message* msg);
    int pushMessage(Message* msg);
    Message* getMessage();

private:
    pthread_mutex_t mMutex;
    pthread_cond_t mCond;
    std::list<Message*> mMessageQueue;
};


#endif
