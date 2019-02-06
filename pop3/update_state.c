//
// Created by timan on 2018-11-26.
//

#include "update_state.h"
#include "../netbuffer.h"
#include "../mailuser.h"
#include "../server.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>


void run_update_state(int fd, mail_list_t list, int* string_check) {
    destroy_mail_list(list);
    (*string_check) += robust_send_string(fd, "%s\r\n", "+OK POP3 server signing off (maildrop empty)");
}
