//
// Created by timan on 2018-11-21.
//
#include "../netbuffer.h"
#include "../mailuser.h"
#include "../server.h"
#include "client_initiation.h"
#include "session_initiation.h"
#include "mail_transaction.h"
#include "message_contents.h"
#include "receipt_specification.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>

/*
 * Receipt Specification
 * RCPT
 * S: 250, 251 (but see Section 3.4 for discussion of 251 and 551)
 *
 * E: 552 Requested mail action aborted: exceeded storage allocation
 *  : 451 Requested action aborted: error in processing
 *  : 551 User not local; please try <forward-path>
 *  : 450 Requested mail action not taken: mailbox unavailable (e.g.,
      mailbox busy or temporarily blocked for policy reasons)
 *  : 452 Requested action not taken: insufficient system storage
 *  : 550 Requested action not taken: mailbox unavailable (e.g., mailbox
      not found, no access, or command rejected for policy reasons)
 *  : 553 Requested action not taken: mailbox name not allowed (e.g.,
      mailbox syntax incorrect)
 *  : 503 Failure: Sender already specified
 *  : 455 Server unable to accommodate parameters
 *  : 555 Invalid MAIL FROM parameters
 */
void run_receipt_specification (int fd , char* out, user_list_t* forward_path, int* string_check) {

    // ** this step can be repeated any number of times

    printf("in receipt_specification \n");

    const char ch = ':';
    char *rcptLine;

    rcptLine = strchr(out, ch);
    printf("rcptLine:%s", rcptLine);        // rcptLine = ": <rcptMailbox>"
    rcptLine += 1;

    char *rcptMailbox;
    const char leftBracket = '<';
    rcptMailbox = strchr(rcptLine, leftBracket);  // look for '<'

    if (rcptMailbox == NULL) {
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 553, "Command syntax incorrect missing <");
        return;
    }
    if (strcasecmp(rcptLine,rcptMailbox) != 0){
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 553, "Command syntax incorrect should be RCPT TO:<>");
        return;
    }

    printf("rcptMailbox:%s", rcptMailbox);      // rcptMailbox = <rcptMailbox>
    rcptMailbox +=1;
    printf("strlen: %lu\r\n", strlen(rcptMailbox));

    const char rightBracket = '>';
    char *rightStr;
    rightStr = strchr(rcptMailbox,rightBracket);


    if (rightStr == NULL) {
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 553, "Command syntax incorrect missing >");
        return;
    }
    size_t checklength = strlen(rightStr);
    if(checklength > 3){                        // 3 = '>' + CR + LF
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 553, "Command syntax incorrect should be RCPT TO:<>");
        return;
    }else {
        rightStr [0] = 0 ;
    };


    printf("valid rcpt?:%s\n", rcptMailbox);      // valid rcpt?:rcptMailbox

    size_t length = strlen(rcptMailbox);


    if (length > 0){

        if (is_valid_user(rcptMailbox, NULL) > 0){
            (*string_check) += robust_send_string(fd, "%d %s\r\n", 250, "rcptMailbox OK");
            add_user_to_list(forward_path, rcptMailbox);
            return;
        }
        else {
            (*string_check) += robust_send_string(fd, "%d %s %s\r\n", 555, "no such user -", rcptMailbox);
            return;
        }
    }
    else {
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 553, "Mailbox parameter is empty");
        return;
    }
}


