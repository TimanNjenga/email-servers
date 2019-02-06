//
// Created by timan on 2018-11-21.
//

#include "netbuffer.h"
#include "server.h"
#include "session_initiation.h"
#include <sys/utsname.h>

/*
 * Session Initiation : Client opens a connection and server responds with a message
 * S : 220 reply text
 * reply text MAY include identification of their software and version information
 * or
 * E: 554 reply text
 * SHOULD provide enough information in the reply text to facilitate debugging
*/
void run_session_initiation ( int fd , int* string_check) {

    // Get uname data
    struct utsname unameData;
    uname(&unameData);


    // Send Welcome Msg
    (*string_check) += robust_send_string(fd, "%d %s \r\n", 220, unameData.nodename);
};

