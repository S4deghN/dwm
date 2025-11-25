#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

void self_restart(const Arg *arg) {
    static char exe_path[128];
    if (exe_path[0] == '\0') {
        int n = readlink("/proc/self/exe", exe_path, sizeof(exe_path));
        if (n == sizeof(exe_path)) {
            fprintf(stderr, "buffer too short to hold path!");
            exe_path[0] = '\0';
            return;
        } else if (n == -1) {
            perror("");
            exe_path[0] = '\0';
            return;
        }
        exe_path[n] = '\0';
    }

    /* after installation of new binary /proc/self/exe points to "<path> (deleted)". */
    /* split the string on the space between path and '(deleted)' */
    char *p = exe_path;
    while (*p > ' ' && *++p);
    *p = '\0';

    char *const argv[] = {exe_path, NULL};

    execvp(argv[0], argv);
}
