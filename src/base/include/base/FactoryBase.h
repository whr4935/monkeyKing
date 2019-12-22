#ifndef FACTORYBASE_H
#define FACTORYBASE_H

#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <base/Global.h>
#include <base/Log.h>

class FactoryBase
{
public:
    typedef void* (*Creator)();

    bool registerPlugin(std::string name, Creator c) {
        mPlugins.insert(name);
        auto it = mCreators.insert({name, c});
        return it.second;
    }

    void* create(std::string name) {
        auto it = mCreators.find(name);
        if (it != mCreators.end()) {
            return it->second();
        }
        return nullptr;
    }

    const std::set<std::string>& plugins() const {
        return mPlugins;
    }

    typedef std::map<std::string,Creator>::const_iterator PluginIterator;
    PluginIterator cbegin() const {
        return mCreators.cbegin();
    }
    PluginIterator cend() const {
        return mCreators.cend();
    }

protected:
    FactoryBase() {}
    virtual ~FactoryBase() {}

private:
    std::map<std::string, Creator> mCreators;
    std::set<std::string> mPlugins;

    DISALLOW_EVIL_CONSTRUCTORS(FactoryBase);
};

#include "PluginManager.h"


#define DECLARE_FACTORY(Interface) class Interface##Factory

#define DEFINE_FACTORY(Interface) \
class Interface##Factory : public FactoryBase, public Singleton<Interface##Factory> \
{ \
    using Singleton<Interface##Factory>::instance; \
    friend Singleton<Interface##Factory>; \
    friend class PluginBase<Interface##Factory>; \
                            \
    Interface##Factory() {  \
        FactoryBase* pFactoryBase = static_cast<FactoryBase*>(this); \
        PluginManager::instance()->registerFactory(Interface::interfaceName(), pFactoryBase); \
    }; \
    ~Interface##Factory() {} \
}

/////////////////////////////
struct PluginDesc {
    int (*init)();
    int version;
    void* handler;
};

#define PLUGIN_DEFINE(_init, _version) \
    struct PluginDesc silent_dream_plugin_desc = { \
    .init 		= _init,    \
    .version 	= _version, \
    }


#endif // FACTORYBASE_H
