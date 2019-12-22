#ifndef _TRANSFER_H
#define _TRANSFER_H

#include <uWebSockets/src/uWS.h>
#include <uWebSockets/src/Group.h>
#include <functional>
#include <thread>

class Transfer {
public:
    Transfer();
    ~Transfer();

    void connectAsync(const char *addr="ws://10.27.11.88:9099");
    int send(std::string filename, const unsigned char* data, int length, bool train, int identity);
    int setLabel(std::map<int, std::string>& labelMap);

    void onConnection(std::function<void(void)> handler) {
        mConnectionHandler = handler;
    }

    void onMessage(std::function<void(char*data, size_t length, bool isText)> handler) {
        mMessageHandler = handler;
    }

private:
    uWS::Hub *mHub = nullptr;
    std::thread *mThread = nullptr;
    uWS::WebSocket<uWS::CLIENT> *mSocket = nullptr;

    std::function<void()> mConnectionHandler;
    std::function<void(char* data, size_t length, bool isText)> mMessageHandler;
};

#endif
