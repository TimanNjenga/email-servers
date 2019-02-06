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
 * MAIL TRANSACTION
 * MAIL
 * S: 250
 *
 *
 * E: 552 Requested mail action aborted: exceeded storage allocation
 *  : 451 Requested action aborted: error in processing
 *  : 452 Requested action not taken: insufficient system storage
 *  : 550 Requested action not taken: mailbox unavailable (e.g., mailbox
      not found, no access, or command rejected for policy reasons)
 *  : 553 Requested action not taken: mailbox name not allowed (e.g.,
      mailbox syntax incorrect)
 *  : 503 Failure: Sender already specified
 *  : 455 Server unable to accommodate parameters
 *  : 555 Invalid MAIL FROM parameters
 */
char* run_mail_transaction (int fd , char* out, char* reverse_path, int* string_check) {
    // out = "MAIL FROM: <me@ubc.edu>"
    if(reverse_path != NULL) {
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 503, "Sender already specified Finish Sending or QUIT");
        return reverse_path ;
    }
    int reverse_path_size = 30;
    reverse_path = (char *) malloc((size_t) reverse_path_size);

    printf("in mail_transaction \n");

    const char ch = ':';
    char *mailFromLine;

    mailFromLine = strchr(out, ch);
    printf("mailFromLine:%s", mailFromLine);        // mailFromLine = ":<sourceMailbox>"
    mailFromLine += 1;

    const char leftBracket = '<';
    char *sourceMailbox;

    sourceMailbox = strchr(mailFromLine, leftBracket);
    if (sourceMailbox == NULL) {
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 553, "Mailbox syntax incorrect missing <");
        return NULL ;
    }
    if (strcasecmp(mailFromLine,sourceMailbox) != 0){
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 553, "Command syntax incorrect should be MAIL FROM:<>"); //**
        return NULL ;
    };

    printf("sourceMailbox:%s", sourceMailbox);      // sourceMailbox = <me@ubc.edu>
    sourceMailbox +=1;

    const char rightBracket = '>';
    char *rightStr;
    rightStr = strchr(sourceMailbox,rightBracket);

    if (rightStr == NULL) {
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 553, "Mailbox syntax incorrect missing >");
        return NULL ;
    }
    size_t checklength = strlen(rightStr);
    if(checklength > 3){                                 // 3 = '>' + CR + LF
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 553, "Command syntax incorrect should be MAIL FROM:<>");
        return NULL;
    }else {
        rightStr [0] = 0 ;
    };

    printf("valid user?: %s\r\n", sourceMailbox);       // to see if trimmed properly

    size_t length = strlen(sourceMailbox);


    if (length > 0){
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 250, "sourceMailbox OK");
        strcpy(reverse_path ,sourceMailbox);
        return reverse_path ;
    }
    else {
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 553, "Mailbox parameter is empty");
        return NULL ;
    }

}


