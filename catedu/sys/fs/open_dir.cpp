#include "open_dir.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

void catedu::sys::open(const char *path)
{
#if defined(_WIN32)
    ShellExecuteA(NULL, "explore", path, NULL, NULL, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    pid_t pid = fork();
    if (pid == 0) {
        execlp("open", "open", path, (char *)nullptr);
        _exit(127);
    }
#elif defined(__linux__)
    pid_t pid = fork();
    if (pid == 0) {
        execlp("xdg-open", "xdg-open", path, (char *)nullptr);
        _exit(127);
    }
#endif
}
