/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef A_DEBUG_H_

#define A_DEBUG_H_

#include <string.h>

#include <foundation/ABase.h>
#include <foundation/AString.h>
#include <utils/log.h>

namespace android {

#define LITERAL_TO_STRING_INTERNAL(x)    #x
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)

#define CHECK(condition)                                \
    LOG_ALWAYS_FATAL_IF(                                \
            !(condition),                               \
            "%s",                                       \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__)    \
            " CHECK(" #condition ") failed.")

#define MAKE_COMPARATOR(suffix,op)                          \
    template<class A, class B>                              \
    AString Compare_##suffix(const A &a, const B &b) {      \
        AString res;                                        \
        if (!(a op b)) {                                    \
            res.append(a);                                  \
            res.append(" vs. ");                            \
            res.append(b);                                  \
        }                                                   \
        return res;                                         \
    }

MAKE_COMPARATOR(EQ,==)
MAKE_COMPARATOR(NE,!=)
MAKE_COMPARATOR(LE,<=)
MAKE_COMPARATOR(GE,>=)
MAKE_COMPARATOR(LT,<)
MAKE_COMPARATOR(GT,>)

#define CHECK_OP(x,y,suffix,op)                                         \
    do {                                                                \
        AString ___res = Compare_##suffix(x, y);                        \
        if (!___res.empty()) {                                          \
            AString ___full =                                           \
                __FILE__ ":" LITERAL_TO_STRING(__LINE__)                \
                    " CHECK_" #suffix "( " #x "," #y ") failed: ";      \
            ___full.append(___res);                                     \
                                                                        \
            LOG_ALWAYS_FATAL("%s", ___full.c_str());                    \
        }                                                               \
    } while (false)

#define CHECK_EQ(x,y)   CHECK_OP(x,y,EQ,==)
#define CHECK_NE(x,y)   CHECK_OP(x,y,NE,!=)
#define CHECK_LE(x,y)   CHECK_OP(x,y,LE,<=)
#define CHECK_LT(x,y)   CHECK_OP(x,y,LT,<)
#define CHECK_GE(x,y)   CHECK_OP(x,y,GE,>=)
#define CHECK_GT(x,y)   CHECK_OP(x,y,GT,>)

#define TRESPASS() \
        LOG_ALWAYS_FATAL(                                       \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__)            \
                " Should not be here.");

struct ADebug {
    enum Level {
        kDebugNone,             // no debug
        kDebugLifeCycle,        // lifecycle events: creation/deletion
        kDebugState,            // commands and events
        kDebugConfig,           // configuration
        kDebugInternalState,    // internal state changes
        kDebugAll,              // all
        kDebugMax = kDebugAll,

    };

    // parse the property or string to get the debug level for a component name
    // string format is:
    // <level>[:<glob>][,<level>[:<glob>]...]
    // - <level> is 0-5 corresponding to ADebug::Level
    // - <glob> is used to match component name case insensitively, if omitted, it
    //   matches all components
    // - string is read left-to-right, and the last matching level is returned, or
    //   the def if no terms matched
    //static Level GetDebugLevelFromProperty(
            //const char *name, const char *propertyName, Level def = kDebugNone);
    //static Level GetDebugLevelFromString(
            //const char *name, const char *value, Level def = kDebugNone);

    // remove redundant segments of a codec name, and return a newly allocated
    // string suitable for debugging
    static char *GetDebugName(const char *name);
};

}  // namespace android

#endif  // A_DEBUG_H_

