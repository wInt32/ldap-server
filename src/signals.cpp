#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

extern "C" {
    // MUST be initialized before registering signal handlers
    int signal_pipe[2] = {-1};

    static const char sigerr_msg[] = "critical error: could not write to signal pipe\n";
    extern void signal_handler(int sig) {
        int res = ::write(signal_pipe[1], &sig, sizeof(sig));

        if (res < 0) {
            ::write(STDIN_FILENO, sigerr_msg, sizeof(sigerr_msg));
            std::quick_exit(1);
        }
    }
}

void set_signal_handlers() {
    int res = ::pipe(signal_pipe);
    if (res < 0) {
        ::perror("pipe()");
        std::exit(1);
    }

    // TODO: use sigaction
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);
    std::signal(SIGPIPE, SIG_IGN);
}