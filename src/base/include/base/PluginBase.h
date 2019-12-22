#ifndef _PLUGIN_BASE_H
#define _PLUGIN_BASE_H

#include <base/Global.h>

template <typename Factory>
class PluginBase
{
public:
    typedef void* (*Creator)();

    static bool registerPlugin(std::string name, Creator creator) {
        return Factory::instance()->registerPlugin(name, creator);
    }

protected:
    PluginBase() {}
    virtual ~PluginBase() {}

private:
    DISALLOW_EVIL_CONSTRUCTORS(PluginBase);
};

#define INTERFACE_META(Interface) \
static std::string interfaceName() { \
    std::string s = #Interface;            \
    return str_tolower(s);            \
}

#define PLUGIN_META(Interface, Name) \
static std::string name() {  \
    std::string s = #Interface "." #Name;  \
    return str_tolower(s);            \
}							 \
                             \
static void* creator() { \
    return new Name##Interface(); 		\
}

#endif
