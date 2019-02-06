//
// Created by timan on 2018-11-21.
//

#ifndef _MESSAGE_CONTENTS_H
#define _MESSAGE_CONTENTS_H

#include "../netbuffer.h"

char* find_message_contents (int fd, char* out, net_buffer_t nb, char* mail_data, int* string_check);

#endif //_MESSAGE_CONTENTS_H
