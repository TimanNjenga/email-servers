//
// Created by timan on 2018-11-26.
//

#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"
#include "update_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <limits.h>
#define MAX_LINE_LENGTH 1024

/*
 * STAT (message count information)  // -DONE
 * LIST (message listing - with or without arguments) // -DONE
 * RETR (message content retrieval) // -DONE
 * DELE (delete message) // DONE
 * RSET (undelete messages)  // DONE
 * NOOP (no operation) - DONE
 * QUIT (session termination) - Enters the update state - DONE
*/

/*
 * NO Operation
 * NOOP
 *
 * This command does not affect any parameters or previously entered commands.
 * It specifies no action other than that the receiver send a "+OK" reply.
 *
 */

struct mail_item {
    char file_name[NAME_MAX];
    size_t file_size;
    unsigned int deleted:1;
};
struct mail_list {
    struct mail_item item;
    struct mail_list *next;
};

void no_operation (int fd, int* string_check) {
    (*string_check) += robust_send_string(fd, "%s\r\n", "+OK");
}

int validity_check(char* out){
    char valid_strings[9][6]  = {"QUIT\r\n","NOOP\r\n","STAT\r\n","LIST ","LIST\r\n","RETR ","DELE ","RSET\r\n","\0"};

    int check = 0;
    for(int i = 0; i < 8; i++) {
        if (strncasecmp(out,valid_strings[i],5) == 0){
            check = 1;
        }
    }
    return  check;
}
mail_item_t my_get_mail_item(mail_list_t list, int i){
    unsigned int pos = 0;

    while (list){
        if(i == pos){
            return &(list->item);
        }
        pos++;
        list = list->next;
    }
    return NULL;
}
unsigned int my_get_mail_count(mail_list_t list) {
    unsigned int rv = 0;
    while (list) {
        rv++;
        list = list->next;
    }
    return rv;
}


void run_transaction_state (char* name, char* out , int fd ,net_buffer_t nb) {

    printf("In transaction_state \n");
    printf("user_name :%s\r\n", name);

    int string_check = 0;

    mail_list_t list =  load_user_mail(name);

    while (string_check >= 0) {

        int valid = validity_check(out);

        if (valid == 0 ){
            string_check += robust_send_string(fd, "%s\r\n", "-ERR  invalid command ");
            string_check += robust_nb_read_line(nb, out);
            continue;
        }
        if (strcasecmp(out, "QUIT\r\n") == 0){
            printf("In QUIT \n");
            run_update_state(fd, list, &string_check) ;
            break;
        }
        if (strcasecmp(out, "NOOP\r\n") == 0){
            size_t check_length = strlen(out);
            if(check_length > 6){
                string_check += robust_send_string(fd, "%s\r\n", "-ERR noop invalid command ");
            }else {
                no_operation(fd , &string_check);
            }
        }
        if (strncasecmp(out, "STAT", 4) == 0) {
            printf("In Stat \n");
            unsigned int count = get_mail_count(list);
            size_t size = get_mail_list_size(list);
            string_check += robust_send_string(fd, "%s%d%s%lu\r\n", "+OK ", count," ", size);
        }
        if (strncasecmp(out, "LIST", 4) == 0) {
            printf("In List \n");
            const char ch = ' ';
            char *scan_listing;
            unsigned int count = my_get_mail_count(list);
            unsigned int without_deleted_count = get_mail_count(list);
            scan_listing = strchr(out, ch);
            if (scan_listing == NULL){
                string_check += robust_send_string(fd, "%s%d%s\r\n","+OK ",without_deleted_count, " messages");
                for (unsigned int i = 0; i < count ; i++){
                    mail_item_t item = my_get_mail_item(list,i) ;
                    if (item != NULL ) {
                        if (item->deleted == 0){
                            size_t item_size = get_mail_item_size(item);
                            string_check += robust_send_string(fd, "%d%s%lu\r\n", i+1," ", item_size);
                        }
                    }
                }
                string_check += robust_send_string(fd, "%s\r\n", ".");
            }else {
                scan_listing += 1;
                size_t length = strlen(scan_listing) - 1; //REMOVE CR strlen doesnt take LF
                int full  = 0;
                while(length > 1){
                    int number = ( *scan_listing - '0');
                    full = (full * 10) + number ;
                    scan_listing += 1;
                    length -=1;
                }
                unsigned int pos = full + UINT_MAX +1; // covert char* to signed int then unsigned
                mail_item_t item = get_mail_item(list,(pos-1)) ;
                if (item) {
                    size_t item_size = get_mail_item_size(item);
                    string_check += robust_send_string(fd, "%s%d%s%lu\r\n", "+OK ", pos," ", item_size);
                }else {
                    string_check += robust_send_string(fd, "%s\r\n", "-ERR no such item ");
                }
            }
        }
        if (strncasecmp(out, "RETR ", 5) == 0) {
            printf("In Retr \n");
            const char ch = ' ';
            char *scan_listing;
            scan_listing = strchr(out, ch);
            if (scan_listing != NULL){
                scan_listing += 1 ;
                size_t length = strlen(scan_listing) - 1; //REMOVE CR strlen doesnt take LF
                int full = 0;
                if (length > 1){
                    while(length > 1){
                        int number = ( *scan_listing - '0');
                        full = (full * 10) + number ;
                        scan_listing += 1;
                        length -=1;
                    }
                    unsigned int pos = full + UINT_MAX +1; // covert char* to signed int then unsigned
                    mail_item_t item = get_mail_item(list,(pos-1)) ;
                    if (item) {
                        size_t item_size = get_mail_item_size(item);
                        string_check += robust_send_string(fd, "%s%lu\r\n", "+OK ", item_size);
                        const char* filename = get_mail_item_filename(item);
                        FILE* file = fopen(filename, "r"); /* should check the result */
                        if (file!= NULL) {
                            char line[MAX_LINE_LENGTH];
                            while (fgets(line, sizeof(line), file)) {
                                printf("%s", line);
                                string_check += robust_send_string(fd, "%s",line);
                            }
                            printf("%s", "\r\n");
                            string_check += robust_send_string(fd, "%s","\r\n");
                            fclose(file);
                        }else {string_check += robust_send_string(fd, "%s\r\n", "-ERR no such file ");}
                    }else {string_check += robust_send_string(fd, "%s\r\n", "-ERR no such item ");}
                }else {string_check += robust_send_string(fd, "%s\r\n", "-ERR missing message-number ");}
            }else {string_check += robust_send_string(fd, "%s\r\n", "-ERR missing message-number ");}

        }
        if (strncasecmp(out, "DELE ", 5) == 0) {
            printf("In Dele \n");
            const char ch = ' ';
            char *scan_listing;
            scan_listing = strchr(out, ch);
            if (scan_listing != NULL){
                scan_listing += 1 ;
                size_t length = strlen(scan_listing) - 1; //REMOVE CR strlen doesnt take LF
                int full = 0;
                if (length > 1){
                    while(length > 1){
                        int number = ( *scan_listing - '0');
                        full = (full * 10) + number ;
                        scan_listing += 1;
                        length -=1;
                    }
                    unsigned int pos = full + UINT_MAX +1; // covert char* to signed int then unsigned
                    mail_item_t item = get_mail_item(list,(pos-1)) ;
                    if (item) {
                        printf("Deleting message %d%s", pos, "\r\n");
                        mark_mail_item_deleted(item);
                        string_check += robust_send_string(fd, "%s%d%s\r\n", "+OK message ", pos , " deleted");
                    }else {string_check += robust_send_string(fd, "%s\r\n", "-ERR no such message ");}
                }else {string_check += robust_send_string(fd, "%s\r\n", "-ERR missing message-number ");}
            }else {string_check += robust_send_string(fd, "%s\r\n", "-ERR missing message-number ");}
        }
        if (strncasecmp(out, "RSET\r\n", 5) == 0) {
            printf("In Rset \n");
            reset_mail_list_deleted_flag(list);
            unsigned int count = get_mail_count(list);
            string_check += robust_send_string(fd, "%s%d%s\r\n", "+OK maildrop has ", count," messages");
        }
        string_check += robust_nb_read_line(nb, out);
    }
}
