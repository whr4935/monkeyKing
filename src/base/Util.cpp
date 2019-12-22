#include <sys/stat.h>
#include <libgen.h>
#include <dirent.h>
#include <Base/Util.h>
#include <Base/Log.h>

static char cwd[PATH_MAX];

int initRootDir()
{
    char path[MAX_PATH] = {0};
    int err = ::readlink("/proc/self/exe", path, MAX_PATH);
    if (err < 0) {
        LOGE("readlink get process dir failed:%s", strerror(errno));
        return -1;
    }

    char *dir = dirname(path);
//    PRINT("Root dir:%s", path);

    err = chdir(dir);
    if (err < 0) {
        LOGE("chdir failed:%s", strerror(errno));
        return -1;
    }

    if (getcwd(cwd, PATH_MAX) < 0) {
        LOGE("getcwd failed:%s", strerror(errno));
        return -1;
    }

    return 0;
}

const char* getRootDir()
{
    return cwd;
}

int ensureDirectoryExist(const char *path)
{
    DIR* dir = opendir(path);
    if (dir == nullptr) {
        int err = mkdir(path, 0755);
        if (err < 0) {
            LOGE("mkdir faield:%s", strerror(errno));
            return -1;
        }
    } else {
        closedir(dir);
    }

    return 0;
}

bool isDaemonRunning()
{
    int fd;

    if ((fd = open("run/silentdream.pid", O_RDWR)) < 0) {
        return false;
    }

    struct flock fl;
    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    if (fcntl(fd, F_GETLK, &fl) == 0) {
        if (fl.l_type == F_UNLCK)
            return false;
    }

    return true;
}





