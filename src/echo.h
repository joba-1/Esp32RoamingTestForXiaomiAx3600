#ifndef ECHO_H
#define ECHO_H

#define ECHO_TIMEOUT_MS 1500

// Call this to start an echo task
// It will connect to a tcp echo service on the given host and set the status according to the connection state
void startEcho( const char *host, uint16_t port, bool *echoSuccess, char *status );

#endif
