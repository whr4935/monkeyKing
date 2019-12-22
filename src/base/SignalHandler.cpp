#include <execinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sstream>
#include <iomanip>

#include <Base/Global.h>
#include <Base/Log.h>
#include "SignalHandler.h"
#include "CallStack.h"


SignalHandler* SignalHandler::self = nullptr;
pthread_once_t once_control;

SignalHandler& SignalHandler::instace()
{
    if (self == nullptr) {
        pthread_once(&once_control, [] {
            self = new SignalHandler();
        });
    }

    return *self;
}


SignalHandler::SignalHandler()
{
}

SignalHandler::~SignalHandler()
{
}

inline void SignalHandler::setCustomSignalHandler(sigset_t set, std::function<void(int)> handler)
{
    mCustomSignalMask = set;
    mCustomSignalHandler = handler;
}

int SignalHandler::install(Loop* loop)
{
    int ret;

    mLoop = loop;

    ret = initCustomSignalHandlers();
    assert(ret == 0);

    ret = initPlatformSignalHandlers();
    assert(ret == 0);

    return 0;
}
////////////////////////////////////////
int SignalHandler::initCustomSignalHandlers()
{
    sigemptyset(&mSignalMask);
    sigaddset(&mSignalMask, SIGCHLD);
    sigaddset(&mSignalMask, SIGPIPE);
    sigaddset(&mSignalMask, SIGINT);
    sigaddset(&mSignalMask, SIGQUIT);
    sigaddset(&mSignalMask, SIGTERM);

    sigorset(&mSignalMask, &mSignalMask, &mCustomSignalMask);

    int err;
    if ((err = pthread_sigmask(SIG_BLOCK, &mSignalMask, &mOldSignalMask)) != 0) {
        LOGE("pthread_sigmask failed:%s", strerror(err));
        return -1;
    }

    err = pthread_create(&mSignalTid, nullptr, customSignalThreadFn, this);
    if (err != 0) {
        LOGE("create signal thread failed:%s", strerror(err));
        return -1;
    }

    return err;
}

void* SignalHandler::customSignalThreadFn(void *arg)
{
   SignalHandler* s = static_cast<SignalHandler*>(arg);
   pthread_detach(pthread_self());

   sigset_t signalMask = s->mSignalMask;
   sigset_t customSignalMask = s->mCustomSignalMask;

   int err, signo;
   for (;;) {
       err = sigwait(&signalMask, &signo);
       if (err != 0) {
           LOGE("sigwait failed:%s", strerror(err));
           break;
       }

       if (sigismember(&customSignalMask, signo) && s->mCustomSignalHandler ) {
           s->mCustomSignalHandler(signo);
           continue;
       }

       switch (signo) {
       case SIGCHLD: {
           int status;
           pid_t pid = waitpid(-1, &status, WNOHANG);
           LOGI("child process(pid:%d) exit with %d", pid, WEXITSTATUS(status));
       }
       break;

       case SIGPIPE: {
           LOGE("SIGPIPE");
       }
       break;

       case SIGINT: {
           LOGE("SIGINT");
           s->mLoop->requestExit();
       }
       break;

       case SIGQUIT: {
           LOGE("SIGQUIT");
           s->mLoop->requestExit();
       }
       break;

       case SIGTERM: {
           LOGE("SIGTERM");
           s->mLoop->requestExit();
       }
       break;

       default:
           LOGW("Unexpected signal:%d", signo);
           s->mLoop->requestExit();
           break;
       }
   }

   return (void*)0;
}

////////////////////////////////////////
int SignalHandler::initPlatformSignalHandlers()
{
    int ret = 0;
    struct sigaction act;

    memset(&act, 0, sizeof(struct sigaction));
    sigemptyset(&act.sa_mask);
    act.sa_flags |= SA_SIGINFO | SA_ONSTACK;
    act.sa_sigaction = unexpectedSignalHandler;

    ret  = sigaction(SIGABRT, &act, NULL);
    ret += sigaction(SIGBUS, &act, NULL);
    ret += sigaction(SIGFPE, &act, NULL);
    ret += sigaction(SIGILL, &act, NULL);
    ret += sigaction(SIGSEGV, &act, NULL);
#if defined(SIGSTKFLT)
    ret += sigaction(SIGSTKFLT, &act, NULL);
#endif
    ret += sigaction(SIGTRAP, &act, NULL);
    assert(ret == 0);

    return 0;
}

void SignalHandler::unexpectedSignalHandler(int signal_number, siginfo_t *info, void* )
{
    static bool handlingUnexpectedSignal = false;
    if (handlingUnexpectedSignal) {
        LOGE("HandleUnexpectedSignal reentered\n");
        _exit(1);
    }
    handlingUnexpectedSignal = true;

    bool has_address = (signal_number == SIGILL || signal_number == SIGBUS ||
                        signal_number == SIGFPE || signal_number == SIGSEGV);

    std::stringstream ss;
    ss << "\n  *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***\n"
       << "  Fatal signal " << signal_number << " (" << getSignalName(signal_number) << "), code "
       << info->si_code << " (" << getSignalCodeName(signal_number, info->si_code) << ")";
    if (has_address) {
        ss << ", fault addr " << std::hex << info->si_addr << "\n";
    }

    instace().dumpstack(ss);
    LOGE("%s", ss.str().c_str());

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    sigemptyset(&action.sa_mask);
    action.sa_handler = SIG_DFL;
    sigaction(signal_number, &action, NULL);
    // ...and re-raise so we die with the appropriate status.
    kill(getpid(), signal_number);
}

const char* SignalHandler::getSignalName(int signal_number)
{
    switch (signal_number) {
      case SIGABRT: return "SIGABRT";
      case SIGBUS: return "SIGBUS";
      case SIGFPE: return "SIGFPE";
      case SIGILL: return "SIGILL";
      case SIGPIPE: return "SIGPIPE";
      case SIGSEGV: return "SIGSEGV";
  #if defined(SIGSTKFLT)
      case SIGSTKFLT: return "SIGSTKFLT";
  #endif
      case SIGTRAP: return "SIGTRAP";
    }
    return "??";
}

const char* SignalHandler::getSignalCodeName(int signal_number, int signal_code)
{
    // Try the signal-specific codes...
    switch (signal_number) {
      case SIGILL:
        switch (signal_code) {
          case ILL_ILLOPC: return "ILL_ILLOPC";
          case ILL_ILLOPN: return "ILL_ILLOPN";
          case ILL_ILLADR: return "ILL_ILLADR";
          case ILL_ILLTRP: return "ILL_ILLTRP";
          case ILL_PRVOPC: return "ILL_PRVOPC";
          case ILL_PRVREG: return "ILL_PRVREG";
          case ILL_COPROC: return "ILL_COPROC";
          case ILL_BADSTK: return "ILL_BADSTK";
        }
        break;
      case SIGBUS:
        switch (signal_code) {
          case BUS_ADRALN: return "BUS_ADRALN";
          case BUS_ADRERR: return "BUS_ADRERR";
          case BUS_OBJERR: return "BUS_OBJERR";
        }
        break;
      case SIGFPE:
        switch (signal_code) {
          case FPE_INTDIV: return "FPE_INTDIV";
          case FPE_INTOVF: return "FPE_INTOVF";
          case FPE_FLTDIV: return "FPE_FLTDIV";
          case FPE_FLTOVF: return "FPE_FLTOVF";
          case FPE_FLTUND: return "FPE_FLTUND";
          case FPE_FLTRES: return "FPE_FLTRES";
          case FPE_FLTINV: return "FPE_FLTINV";
          case FPE_FLTSUB: return "FPE_FLTSUB";
        }
        break;
      case SIGSEGV:
        switch (signal_code) {
          case SEGV_MAPERR: return "SEGV_MAPERR";
          case SEGV_ACCERR: return "SEGV_ACCERR";
        }
        break;
      case SIGTRAP:
        switch (signal_code) {
          case TRAP_BRKPT: return "TRAP_BRKPT";
          case TRAP_TRACE: return "TRAP_TRACE";
        }
        break;
    }
    // Then the other codes...
    switch (signal_code) {
      case SI_USER:     return "SI_USER";
  #if defined(SI_KERNEL)
      case SI_KERNEL:   return "SI_KERNEL";
  #endif
      case SI_QUEUE:    return "SI_QUEUE";
      case SI_TIMER:    return "SI_TIMER";
      case SI_MESGQ:    return "SI_MESGQ";
      case SI_ASYNCIO:  return "SI_ASYNCIO";
  #if defined(SI_SIGIO)
      case SI_SIGIO:    return "SI_SIGIO";
  #endif
  #if defined(SI_TKILL)
      case SI_TKILL:    return "SI_TKILL";
  #endif
    }
    // Then give up...
    return "?";
}

void SignalHandler::dumpstack(std::ostream& os)
{
#if 0
  char* stack[20] = {0};
  int depth = backtrace(reinterpret_cast<void**>(stack), sizeof(stack)/sizeof(stack[0]));
  if (depth){
      char** symbols = backtrace_symbols(reinterpret_cast<void**>(stack), depth);
      if (symbols){
          for(int i = 0; i < depth; i++){
              LOGE("===[%d]:%s", (i+1), symbols[i]);
          }
      }
      free(symbols);
  }

#else

    CallStack c(os, "DEBUG");
#endif
}




