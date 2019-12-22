#include <base/Log.h>
#include "ArgumentParser.h"

static struct ParameterDesc parameterTable[] = {
    {"version", VERSION, 0},
    {"help", HELP, 0},
    {"ls", LS, -1},
    {"status", STATUS, 0},
    {"start", START, 1},
    {"stop", STOP, 1},
};

void to_json(json& j, const ParameterDesc& p) {
    j = json{{"what", p.what}, {"cmd", p.cmd}, {"arg-num", p.argNum}};
}

void from_json(const json& j, ParameterDesc& p) {
    p.what = j.at("what").get<std::string>();
    p.cmd = j.at("cmd").get<CMD>();
    p.argNum = j.at("arg-num").get<int>();
}

///////////////////////////////////////////////////////////
ArgumentParser::ArgumentParser(int argc, char **argv)
    : mArgc(argc)
    , mArgv(argv)
{
    for (size_t i = 0; i < ARRAY_SIZE(parameterTable); ++i) {
        mParameterDescTable.insert({parameterTable[i].what, parameterTable+i});
    }
}

ArgumentParser::~ArgumentParser()
{

}

int ArgumentParser::initCheck()
{
    if (mArgc > 1) {
        mRunMOde = RUN_MODE_CLIENT;
    } else {
        mRunMOde = RUN_MODE_DAEMON;
    }

    if (mArgc > 1) {
        char* argName = mArgv[1];
        auto ret = mParameterDescTable.find(argName);
        if (ret == mParameterDescTable.end()) {
            PRINT("unknown command: %s", argName);
            return -1;
        }

        mCurrentArg = argName;
        mCurrentArgDesc = ret->second;
        if (mCurrentArgDesc->argNum >= 0 && mArgc-2 != mCurrentArgDesc->argNum) {
            PRINT("parameter invalid!");
            return -1;
        }
    }

    return 0;
}

int ArgumentParser::parse(json& result)
{
    int argc = 2;
    std::string argName;
    std::string argBaseName = "arg-";

    result = *mCurrentArgDesc;
    result["arg-num"] = mArgc - argc;
    while (argc < mArgc) {
        argName = argBaseName + std::to_string(argc - 1);
        result[argName] = mArgv[argc];
        ++argc;
    }
    PRINT("command line arguments:");
    PRINT("\t%s", result.dump().c_str());

    return 0;
}
