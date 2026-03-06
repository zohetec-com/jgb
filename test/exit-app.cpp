// google AI : linux c timer callback example
#include <jgb/core.h>
#include <jgb/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

// Timer callback function
void timer_callback(int sig, siginfo_t *si, void *uc) {
    jgb_notice("Timer expired! Signal received: %d\n", sig);
    // 触发段错误
    *((int*)(intptr_t)0x04) = 0x12345678;
    // You can access user data if needed:
    // int *data = (int *)si->si_value.sival_ptr;
    // printf("User data: %d\n", *data);
}

int init_timer() {
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;
    struct sigaction sa;

    // 1. Set up the signal handler for the timer
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timer_callback;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGRTMIN, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    // 2. Create the timer
    sev.sigev_notify = SIGEV_SIGNAL; // Notify by signal
    sev.sigev_signo = SIGRTMIN;      // Use a real-time signal
    sev.sigev_value.sival_ptr = NULL; // No user data in this example, or point to your data
    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
        perror("timer_create");
        exit(EXIT_FAILURE);
    }

    // 3. Arm the timer
    its.it_value.tv_sec = 60;        // Initial expiration in 2 seconds
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 1;     // Subsequent expirations every 1 second (for periodic timer)
    its.it_interval.tv_nsec = 0;

    if (timer_settime(timerid, 0, &its, NULL) == -1) {
        perror("timer_settime");
        exit(EXIT_FAILURE);
    }

    jgb_notice("Timer created and armed. Waiting for timer expirations...\n");
#if 0
    // Keep the main thread alive to receive signals
    sleep(10); 

    // 4. Delete the timer (optional, but good practice)
    if (timer_delete(timerid) == -1) {
        perror("timer_delete");
        exit(EXIT_FAILURE);
    }

    jgb_notice("Timer deleted. Exiting.\n");
#endif
    return 0;
}

static int init(void* conf)
{
    init_timer();
    return 0;
}

jgb_api_t exit_app
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "exit app",
    .init = init,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = nullptr
};
