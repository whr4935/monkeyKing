#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <sstream>
#include <map>
#include <set>
#include <base/Global.h>
#include <base/Log.h>
#include <base/FactoryBase.h>
#include <base/PluginBase.h>

class PluginManager : public Singleton<PluginManager>
{
public:
    using Singleton<PluginManager>::instance;
    friend class Singleton<PluginManager>;

    void loadPlugins();

    bool registerFactory(std::string interfaceName, FactoryBase* interfaceFactory) {
        mInterfaces.insert(interfaceName);
        auto it = mFactories.insert({interfaceName, interfaceFactory});
        return it.second;
    }

    template <typename T=void>
    T* create(std::string pluginName) {
        T* plugin = nullptr;

        std::stringstream ss( pluginName);
        std::string interfaceName;
        std::string name;
        std::getline(ss, interfaceName, '.');
        std::getline(ss, name);

        FactoryBase* factory = findFactory(interfaceName);
        if (factory != nullptr) {
            plugin = static_cast<T*>(factory->create(pluginName));
        }
        return plugin;
    }

    FactoryBase* findFactory(std::string interfaceName);

    const std::set<std::string>& interfaces() const {
        return mInterfaces;
    }

    typedef std::map<std::string, FactoryBase*>::const_iterator InterfaceIterator;
    InterfaceIterator cbegin() const {
        return mFactories.cbegin();
    }

    InterfaceIterator cend() const {
        return mFactories.cend();
    }

private:
    PluginManager() {}
    ~PluginManager() {}

    std::map<std::string, FactoryBase*> mFactories;
    std::set<std::string> mInterfaces;
};


#endif // PLUGINMANAGER_H
