#include <base/PluginManager.h>
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>

void PluginManager::loadPlugins()
{
    std::string dirName = "plugins/";
    std::string pluginName;
    void* dlHandler;
    struct PluginDesc* pluginDesc;

    DIR* plugin_dir = ::opendir(dirName.c_str());
    if (plugin_dir == nullptr) {
        LOGE("opendir failed:%s", strerror(errno));
        return;
    }

    struct dirent* ent;
    while ((ent = ::readdir(plugin_dir)) != nullptr) {
        if (ent->d_name[0]=='.' && (ent->d_name[1]==0 || ent->d_name[1]=='.'))
            continue;

        if (ent->d_type != DT_REG)
            continue;

        pluginName = dirName +  ent->d_name;
        dlHandler = ::dlopen(pluginName.c_str(), RTLD_NOW);
        if (dlHandler == nullptr) {
            LOGE("dlopen error:%s", dlerror());
            continue;
        }

        pluginDesc = (struct PluginDesc*)::dlsym(dlHandler, "silent_dream_plugin_desc");
        if (pluginDesc == nullptr)
            continue;

        if (pluginDesc->init() < 0) {
            LOGE("plugin:%s init failed!", pluginName.c_str());
        }
        pluginDesc->handler = dlHandler;
        LOGV("load plugin: %s success!", ent->d_name);
    }
    ::closedir(plugin_dir);
}

FactoryBase* PluginManager::findFactory(std::string interfaceName) {
    auto it = mFactories.find(interfaceName);
    if (it != mFactories.end()) {
        return it->second;
    }
    return nullptr;
}
