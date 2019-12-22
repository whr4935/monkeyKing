#include <SilentDream/Log.h>
#include "HelloApp.h"

HelloApp::HelloApp()
{

}

HelloApp::~HelloApp()
{
    LOGI("HelloApp dtor");
}

int HelloApp::start()
{
    LOGI("hello start!");
   return 0;
}

int HelloApp::stop()
{
    LOGI("hello stop!");
    return 0;
}

int plugin_init()
{
    HelloApp::registerPlugin(HelloApp::name(), HelloApp::creator);

    return 0;
}




PLUGIN_DEFINE(plugin_init, 1);
