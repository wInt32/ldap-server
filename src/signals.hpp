extern "C" {
    extern int signal_pipe[2];
    extern void signal_handler(int sig);
}

void set_signal_handlers();