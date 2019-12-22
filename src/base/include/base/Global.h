#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <iostream>
#include <sstream>
#include <string>
#include <cctype>
#include <map>
#include <set>
#include <algorithm>

#include "Util.h"

enum RunMode {
    RUN_MODE_DAEMON,
    RUN_MODE_CLIENT,
};

#ifndef MAX_PATH
#define MAX_PATH  260
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

#define DISALLOW_EVIL_CONSTRUCTORS(name) \
    name(const name &); \
    name &operator=(const name &)


template <typename T>
class Singleton
{
public:
    virtual ~Singleton() {}

protected:
    static T* instance() {
        if (self == nullptr) {
            pthread_once(&once_control, [] {
                self = new T();
            });
        }
        return self;
    }

    Singleton() {}

private:
    static T* self;
    static pthread_once_t once_control;

    DISALLOW_EVIL_CONSTRUCTORS(Singleton);
};

template <typename T>
 T* Singleton<T>::self = nullptr;

template <typename T>
pthread_once_t Singleton<T>::once_control = PTHREAD_ONCE_INIT;

static inline std::string str_tolower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); }
                  );
    return s;
}

#endif // GLOBAL_H
