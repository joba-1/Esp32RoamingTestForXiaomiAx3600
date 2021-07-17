#ifndef PINGER_H
#define PINGER_H

// Call this to start a ping task
// It will ping given host and set the status
void startPing( const char *host, bool *pingSuccess );

#endif