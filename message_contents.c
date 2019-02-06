//
// Created by timan on 2018-11-21.
//

#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include "netbuffer.h"
#include "message_contents.h"

#define MAX_LINE_LENGTH 1024

/*
 *
 * Message Contents
 * DATA
 * I: 354 -> data -> S: 250  // 354 Start mail input; end with <CRLF>.<CRLF>
 *
 *                   E: 552 Requested mail action aborted: exceeded storage allocation
 *                    : 554 Transaction failed (Or, in the case of a connection-opening response, "No SMTP service here")
 *                    : 451 Requested action aborted: local error in processing
 *                    : 452 Requested action not taken: insufficient system storage
 *
 *
 *                   E: 450 Requested mail action not taken: mailbox unavailable (e.g.,
      mailbox busy or temporarily blocked for policy reasons)
      550 (rejections for policy reasons)
 * E: 503, 554
 */
char* find_message_contents (int fd, char* out, net_buffer_t nb, char* mail_data, int* string_check) {

    printf("GOT INTO DATA \r\n");
    size_t mail_data_size = 0;
    mail_data = NULL;
    size_t i = 1 ;

    // Send "354 Start mail input; end with <CRLF>.<CRLF>"
    (*string_check) += robust_send_string(fd, "%d %s\r\n", 354, "Start mail input; end with '.' " );

    //read all data lines and add them to a mail_data_buffer ending with "."
    // ** need to fix/ask,  need the second case to make it work for my computer,
    //                     but a proper <CRLF> is created by \r\n .....

    while (strcasecmp(out, ".\r\n") != 0){
        (*string_check) += robust_nb_read_line(nb, out);
        if (mail_data == NULL){
            mail_data_size += strlen(out) ;
            mail_data = malloc(MAX_LINE_LENGTH);
            memset(mail_data, 0, MAX_LINE_LENGTH);
        }else{
            mail_data_size += strlen(out) ;
            mail_data = realloc(mail_data,(i*MAX_LINE_LENGTH));
        }
        strncat(mail_data,out,mail_data_size);
        i++;
    }
    memset(mail_data + (mail_data_size - 1) ,'\0',1);  // Terminate the  end of the data

    if (mail_data_size > 0){
        printf("Message accepted \n");
        printf("%s\r\n", mail_data);
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 250, "Message accepted for Delivery" );
    }else {
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 554, "Transaction failed. Error in DATA handling." );
    }
    return mail_data ;

}

