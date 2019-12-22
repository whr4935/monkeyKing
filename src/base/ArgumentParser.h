#ifndef _ARGUMENT_PARSER_H
#define _ARGUMENT_PARSER_H

#include <base/Global.h>
#include <json.hpp>
using nlohmann::json;

enum CMD {
    VERSION = 100,
    HELP,
    LS,
    STATUS,
    START,
    STOP,
};

struct ParameterDesc {
    std::string what;
    CMD cmd;
    int argNum;
};


class ArgumentParser {
public:
    ArgumentParser(int argc, char **argv);
    ~ArgumentParser();

    int initCheck();
    int parse(json& result);
    RunMode getRunMode() const {
        return mRunMOde;
    }

private:
    int mArgc;
    char **mArgv;

    std::map<std::string, struct ParameterDesc*> mParameterDescTable;
    RunMode mRunMOde = RUN_MODE_CLIENT;
    std::string mCurrentArg;
    struct ParameterDesc* mCurrentArgDesc = nullptr;
};

#endif
