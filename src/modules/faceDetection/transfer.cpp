#include "transfer.h"
#include <android/log.h>

#define LOG_NDEBUG 0
#define LOG_TAG "transfer"

#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, ##__VA_ARGS__)
#define ALOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, ##__VA_ARGS__)
#define ALOGE(...) \
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, ##__VA_ARGS__)


Transfer::Transfer()
{
    mHub = new uWS::Hub();
    if (mHub == nullptr) {
        ALOGE("initialize uWebSockets failed!");
        return;
    }

    mHub->onConnection([this](uWS::WebSocket<uWS::CLIENT> *ws, uWS::HttpRequest) {
            ALOGI("uWebSockets connect success!");
            this->mSocket = ws;
            if (mConnectionHandler) {
                mConnectionHandler();
            }
            });

    mHub->onMessage([this](uWS::WebSocket<uWS::CLIENT> *ws, char *data, size_t length, uWS::OpCode opCode) {
        if (length) {
            this->mMessageHandler(data, length, opCode==uWS::TEXT);
        }
    });

    mHub->onDisconnection([this](uWS::WebSocket<uWS::CLIENT> *ws, int code, char *message, size_t length) {
        ALOGI("uWebSockets disconnect!");
        this->mSocket = nullptr;
    });

    mHub->onError([this](uWS::Group<uWS::CLIENT>::errorType err) {
            ALOGE("Error:%s", (char*)err);
            });
}

Transfer::~Transfer()
{
    if (mSocket) {
        mSocket->close();
    }

    if (mThread) {
        mThread->join();
        delete mThread;
        mThread = nullptr;
    }

    if (mHub) {
        delete mHub;
        mHub = NULL;
    }
}

void Transfer::connectAsync(const char* addr)
{
    ALOGI("connectAsync... (%s)", addr);
    mHub->connect(addr, nullptr);
    mThread = new std::thread([this]{
            this->mHub->run();
            });
}

int Transfer::send(std::string filename, const unsigned char* data, int length, bool train, int identity)
{
    if (mSocket == nullptr)
        return -1;

    std::string opCode;
    if (train) {
        opCode = "train";
    } else {
        opCode = "detect";
    }

    char buf[200];
    sprintf(buf, "C:%s:%s:%d", opCode.c_str(), filename.c_str(), identity);
    mSocket->send(buf);

    int size;
    while (length) {
        size = length > 4096 ? 4096 : length;
        mSocket->send((const char*)data, size, uWS::BINARY);
        data += size;
        length -= size;
    }

    sprintf(buf, "C:close:%s:%d", filename.c_str(), identity);
    mSocket->send(buf);

    return 0;
}

int Transfer::setLabel(std::map<int, std::string>& labelMap)
{
    std::string buf;

    buf = "L:";
    for (auto &p : labelMap) {
        buf += p.second;
        buf += ":";
    }
    buf.erase(buf.end()-1);

    mSocket->send(buf.c_str());

    return 0;
}

