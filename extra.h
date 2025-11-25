#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

void self_restart(const Arg *arg) {
    char exe_path[128];
    int n = readlink("/proc/self/exe", exe_path, sizeof(exe_path));
    if (n == sizeof(exe_path)) {
        fprintf(stderr, "buffer too short to hold path!");
        return;
    } else if (n == -1) {
        perror("");
        return;
    }
    exe_path[n] = '\0';

    char *const argv[] = {exe_path, NULL};

    execvp(argv[0], argv);
}
