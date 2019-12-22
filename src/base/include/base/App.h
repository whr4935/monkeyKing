#ifndef APP_H
#define APP_H

#include "PluginBase.h"
#include "FactoryBase.h"


DECLARE_FACTORY(App);

class App : public PluginBase<AppFactory>
{
public:
    INTERFACE_META(App)

    virtual int start() = 0;
    virtual int stop() = 0;

    ~App() {}

protected:
    App() {}

private:
    DISALLOW_EVIL_CONSTRUCTORS(App);
};

DEFINE_FACTORY(App);



#endif // APP_H
