
#include <stdio.h>
#include <unistd.h>
#include <syslog.h>

int main(void) {

 openlog("slog", LOG_PID|LOG_CONS, LOG_USER);
 syslog(LOG_INFO, "A different kind of Hello world ... ");
 closelog();
}