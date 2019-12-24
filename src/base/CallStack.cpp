#include <iostream>
#include <iomanip>
#include <sstream>

#include <base/Log.h>
#include "CallStack.h"

CallStack::CallStack(std::ostream& os, std::string tag)
{
    buildMap();
    unwind(os, tag);
}

void CallStack::unwind(std::ostream& os, std::string tag)
{
    unw_cursor_t cursor; unw_context_t uc;
    unw_word_t ip;

    const MapEntry *entry = nullptr;

    os << "\n";
    os << std::hex;

    char sym[256];
    unw_word_t offset;
    int step = 0;

    unw_getcontext(&uc);
    unw_init_local(&cursor, &uc);
    while (unw_step(&cursor) > 0) {
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
//        unw_get_reg(&cursor, UNW_REG_SP, &sp);

        if (ip == 0)
            break;

        if (step == 0) {
            os << "Backtrace:\n";
        }

        entry = findMapEntry(ip);

        if (entry) {
            os.fill('0');
            os << "  " << tag << " #" << std::setw(2) << std::right << step << "  " << std::setw(8) << (ip - (unw_word_t)entry->start);
//            os << "  " << tag << " #" << std::setw(2) << std::right << step << "  " << std::setw(8) << ip;
            os.fill(' ');
            os << "  " << std::setw(70) << std::left << entry->name << "  ";
        } else {
            os.fill('0');
            os << tag << "  0x" << std::setw(8) << ip;
            os.fill(' ');
        }
        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0) {
            char* nameptr = sym;
            int status;
            char* demangled = abi::__cxa_demangle(sym, nullptr, nullptr, &status);
            if (status == 0) {
              nameptr = demangled;
            }
            os << "(" << nameptr << "+" << offset <<")\n";

            free(demangled);
        }

        ++step;
    }
}


void CallStack::buildMap()
{
    char path[sizeof(pid_t)*3 + sizeof("/proc//maps") + 1];
    char line[1024];

    snprintf(path, sizeof(path), "/proc/%d/maps", getpid());
    FILE* fp = fopen(path, "r");
    if (fp == NULL) {
        LOGE("open maps file failed:%s", strerror(errno));
        return;
    }

    while(fgets(line, sizeof(line), fp)) {
      MapEntry entry;
      if (ParseLine(line, &entry)) {
        mMaps.push_back(entry);
      }
    }
    fclose(fp);
}

bool CallStack::ParseLine(const char *line, MapEntry *entry)
{
    unsigned long int start;
    unsigned long int end;
    char permissions[5];
    int name_pos;

  // Linux /proc/<pid>/maps lines:
  // 6f000000-6f01e000 rwxp 00000000 00:0c 16389419   /system/lib/libcomposer.so\n
  // 012345678901234567890123456789012345678901234567890123456789
  // 0         1         2         3         4         5
    if (sscanf(line, "%lx-%lx %4s %*x %*x:%*x %*d%n",
               &start, &end, permissions, &name_pos) != 3) {
      return false;
    }

    entry->start = start;
    entry->end = end;
    entry->flags = PROT_NONE;
    if (permissions[0] == 'r') {
      entry->flags |= PROT_READ;
    }
    if (permissions[1] == 'w') {
      entry->flags |= PROT_WRITE;
    }
    if (permissions[2] == 'x') {
      entry->flags |= PROT_EXEC;
    }

    while (isspace(line[name_pos])) {
      name_pos += 1;
    }
    entry->name = line+name_pos;
    if (!entry->name.empty() && entry->name[entry->name.length()-1] == '\n') {
      entry->name.erase(entry->name.length()-1);
    }

//    LOGV("Parsed map: start=%p, end=%p, flags=%x, name=%s",
//          reinterpret_cast<void*>(entry->start), reinterpret_cast<void*>(entry->end),
//          entry->flags, entry->name.c_str());
    if (entry->flags & PROT_EXEC)
        return true;
    return false;
}

const CallStack::MapEntry* CallStack::findMapEntry(uintptr_t addr) const
{
   for (auto &entry : mMaps) {
      if (addr >= entry.start && addr < entry.end) {
          return &entry;
      }
   }

   return nullptr;
}








