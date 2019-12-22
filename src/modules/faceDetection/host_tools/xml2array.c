/*************************************************************************
 > File Name: xml2array.c
 > Created Time: 2017年06月13日 星期二 15时33分05秒
 ************************************************************************/
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

char tmp[4096];

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("usage: xml2array inputfile\n");
        exit(0);
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        printf("open %s failed:%s\n", argv[1], strerror(errno));
        exit(1);
    }

    char outFileName[100] = "a.hpp";
    char arrayName[100] = "haar";
    char *pname = strrchr(argv[1], '/');
    if (pname != NULL) {
        pname = pname+1;
    } else {
        pname = argv[1];
    }

    char *psuffix;
    if ((psuffix = strrchr(argv[1], '.')) == NULL) {
        psuffix = argv[1] + strlen(argv[1]);
    }
    strncpy(outFileName, pname, psuffix-pname);
    outFileName[psuffix-pname] = '\0';
    strcpy(arrayName, outFileName);

    strcat(outFileName, ".hpp");
    
    /* printf("outFileName: %s\n", outFileName); */

    int out_fd;
    if ((out_fd = open(outFileName, O_CREAT|O_TRUNC|O_WRONLY, 0666)) < 0) {
        printf("open output file %s failed:%s\n", outFileName, strerror(errno));
        exit(0);
    }

    char buffer[1024];
    int rd_size;

    char *p;
    int out_size;

    int count = 0;

    out_size = sprintf(tmp, "const char %s[] = {\n", arrayName);
    write(out_fd, tmp, out_size);

    for (;;) {
        memset(buffer, 0, sizeof(buffer)/sizeof(*buffer));
        rd_size = read(fd, buffer, 1024);
        if (rd_size <= 0) {
            /* printf("read return with:%d\n", rd_size); */
            break;
        }

        p = buffer;
        memset(tmp, 0, sizeof(tmp)/sizeof(*tmp));
        out_size = 0;
        while (rd_size--) {
            out_size += sprintf(tmp+out_size, "0x%.2X, ", *p++);
            if ((++count&15)==0) {
                out_size +=sprintf(tmp+out_size, "\n");
            }
        } 

        if (write(out_fd, tmp, out_size) != out_size) {
            printf("fwrite error:%s\n", strerror(errno));
            exit(1);
        }
    }

    out_size = sprintf(tmp, "\n};\n");
    write(out_fd, tmp, out_size);


    fsync(out_fd);
    close(out_fd);
    close(fd);

    return 0;

}
