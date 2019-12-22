#ifndef HELLOAPP_H
#define HELLOAPP_H

#include <SilentDream/App.h>

class HelloApp : public App
{
public:
    PLUGIN_META(App, Hello)

    int start();
    int stop();


private:
    HelloApp();
    ~HelloApp();
};

#endif // HELLOAPP_H
