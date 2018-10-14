#include <iostream>
#include <utils/log.h>
#include <utils/RefBase.h>
#include <memory>
#include <utils/Vector.h>
#include <foundation/ALooper.h>
#include <foundation/AHandler.h>
#include <foundation/AMessage.h>

#include <gtest/gtest.h>

using namespace android;
class Base : public android::RefBase
{
public:
    Base() {}
    void fun() {
        ALOGI("hello base");
    }
};


class MyHandler : public AHandler
{
public:
    void onMessageReceived(const sp<AMessage>& msg) override {
        uint32_t what = msg->what();
        switch (what) {
        case 1:
            std::cout << "hello message 1" << std::endl;
            break;
        }

    }
};


int cmain(int argc, char *argv[])
{
    ALOGI("hello");

    sp<Base> p = new Base();
    p->fun();


    std::shared_ptr<Base> p2(new Base());
    p2->fun();

    Vector<int> v;
    v.push(1);
    v.push(2);
    v.push(3);
    v.push(4);
    v.insertAt(5, 0);
    //std::cout << v.size() << std::endl;

    //std::cout << *v.begin() << std::endl;

    for (auto i : v) {
        std::cout << i << " ";
    }
    std::cout << std::endl;


    int b = 20;
    int a = 10;
    //int& a = b;
    //decltype((b)) a = b;
    static_cast<int&>(a) = b;
    std::cout << a << std::endl;
    std::cout << "&a = " << &a << "  &b = " << &b << std::endl;

    sp<ALooper> th = new ALooper;
    sp<MyHandler> mh = new MyHandler;
    th->registerHandler(mh);
    sp<AMessage> msg = new AMessage(1, mh->id());
    th->start();
    msg->post();

    sleep(1);


    return 0;
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


