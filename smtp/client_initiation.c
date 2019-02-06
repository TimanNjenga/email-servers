//
// Created by timan on 2018-11-21.
//

#include <stdlib.h>
#include "../netbuffer.h"
#include "../server.h"
#include "client_initiation.h"


/*
 * Client initiation ( As per assignment specifications does not support EHLO )
 * HELO
 *    S: 250 Hello (client domain name) pleased to meet you  // ex. 250 Hello crepes.fr, pleased to meet you
 * || E: 504  - Command parameter not implemented
 * || E: 502  - unsupported commands (e.g., EHLO, RSET, VRFY, EXPN, HELP)
 * || E: 500  - Invalid commands
*/

int run_client_initiation (int fd , char* out, int count, int* string_check) {

    int counter = count;

    if (strcasecmp(out, "QUIT\r\n") == 0) {
        size_t checklength = strlen(out);
        if(checklength > 6){
            (*string_check) += robust_send_string(fd, "%d %s\r\n", 500, "Command syntax incorrect for QUIT");
            return 0;
        }
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 221, "Goodbye!");
        return 476;
    }

    if (strcasecmp(out, "NOOP\r\n") == 0) {
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 250, "OK!");
        return counter;
    }
    else if ((strncasecmp(out, "HELO ", 5) == 0) && (out[5] != ' ')){// looks for 'HELO  ' in first 5 chars
        printf("GOT INTO HELO \n");
        // extract domain name from command
        const char ch = ' ';                       // looks for ' ' in command
        char *domainName = NULL;

        domainName = strchr(out, ch);

        if ((domainName == NULL) || (strlen(domainName) <= 3)){   //  2 = SPACE + LF
            (*string_check) += robust_send_string(fd, "%d %s\r\n", 504, "Command parameter not implemented");
            return 0;
        }

        domainName += 1;        // increase ptr to get rid of space
        printf("domainName:%s", domainName);

        // Send "250 OK. Hello domainName."
        (*string_check) += robust_send_string(fd, "%d %s %s", 250, "OK, Hello", domainName);
        counter++ ;
        return counter;
    }
    else if ((strncasecmp(out, "EHLO", 4) == 0)||(strncasecmp(out, "RSET", 4) == 0)||
             (strncasecmp(out, "VRFY", 4) == 0)||(strncasecmp(out, "EXPN", 4) == 0)||
             (strncasecmp(out, "HELP", 4) == 0)){
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 502, "Unsupported command");
        return 0 ;
    }
    else {
        (*string_check) += robust_send_string(fd, "%d %s\r\n", 500, "Command not recognized");
        return 0;
    }

}
