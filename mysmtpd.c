#include "netbuffer.h"
#include "mailuser.h"
#include "server.h"
#include "smtp/client_initiation.h"
#include "smtp/session_initiation.h"
#include "smtp/mail_transaction.h"
#include "smtp/message_contents.h"
#include "smtp/receipt_specification.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <time.h>
#include <locale.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

static void handle_client(int fd);
struct user_list {
    char *user;
    struct user_list *next;
};
typedef struct message_parameters {
    char *message;
    size_t message_size;
} mp, *message_parameters_ptr;

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
        return 1;
    }
    printf("Before run_server \n");
    run_server(argv[1], handle_client);

    return 0;
}

/*
 * NO Operation
 * NOOP
 *
 * This command does not affect any parameters or previously entered commands.
 * It specifies no action other than that the receiver send a "250 OK" reply.
 *
 *  S: 250
 */
void no_operation (int fd, int* string_check) {
    (*string_check) += robust_send_string(fd, "%d %s\r\n", 250, "OK!");
}

/*
 * Session Termination(quit)
 * QUIT
 * S: 221
 *
 */
void quit (int fd , int* string_check) {
    (*string_check) += robust_send_string(fd, "%d %s\r\n", 221, "Goodbye!");
}

char* getTimestamp (){

    size_t  buff_size = 16 ;
    char* buff = NULL;
    buff = (char *) malloc((size_t) buff_size);

    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    strftime (buff,buff_size,"%G%m%d%H%M%S",timeinfo);
    puts(buff);
    return buff;
}




char* help_concat (char* variable, char* message, size_t message_size){

    message_size += strlen(variable) ;
    message = realloc(message, message_size+MAX_LINE_LENGTH);
    char * ptr = message;
    ptr  += message_size;
    memset(ptr,0,MAX_LINE_LENGTH-message_size);
    strcat(message, variable);

    message_size += strlen("\n") ;
    message = realloc(message, message_size);
    strcat(message, "\n");

    return message;
}




message_parameters_ptr handle_make_message (
        char *mail_data, char *reverse_path,user_list_t forward_path, message_parameters_ptr mp){

    size_t  message_size = 0;
    char* message = NULL;
    char* timestamp = NULL;
    message = malloc(MAX_LINE_LENGTH);
    message_size = strlen(message);
    memset(message, 0, message_size);
    message_size = strlen(message);

    message = help_concat(reverse_path,message,message_size);
    message_size = strlen(message);

    while(forward_path){
        message = help_concat(forward_path->user,message,message_size);
        message_size = strlen(message);
        forward_path = forward_path->next;
    }

    timestamp = getTimestamp();
    message = help_concat(timestamp,message,message_size);
    message_size = strlen(message);

    message = help_concat(mail_data,message,message_size);
    message_size = strlen(message);

    memset(message + (message_size - 1) ,'\0',1);  // Terminate the  end of the message

    mp->message_size = message_size;
    mp->message = malloc(message_size);
    memset(mp->message, 0, message_size);
    strncpy(mp->message , message, message_size);

    free(timestamp);
    free(message);
    return mp;


}

int write_to_file(message_parameters_ptr mp, char* template) {
    printf("Enter write_to_file \n");
    size_t  message_size = mp->message_size;
    char* message = mp->message;
    printf("message - %s\n", message);

    int rn;
    rn = mkstemp(template);
    if (rn < 0) {
        printf("Error at making!!!\n");
        unlink(template);
        close(rn);
    } else {
        printf("filename - %s\n", template);
        int wr  = (int) write(rn,message, message_size) ;
        if (wr < 0){
            printf("Error at writing!!!\n");
            unlink(template);
            close(rn);
            return rn;
        }else{
            int re  = (int) read(rn,message,message_size) ;
            if ( re < 0){
                printf("Error at reading!!!\n");
                unlink(template);
                close(rn);
                return rn;
            }
        }
        printf("Successfully written message!!!\n");
        close(rn);
        return rn;
    }
    return rn;
}

void handle_client(int fd) {

    int string_check = 0 ;
    run_session_initiation (fd, &string_check) ;   // Client opens a connection and server responds with a message

    // Create nb to receive what client types on command line
    char out[MAX_LINE_LENGTH];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);
    char *reverse_path = NULL;
    user_list_t forward_path = create_user_list();
    char *mail_data = NULL;
    robust_nb_read_line(nb, out);
    int rn;


    int counter = 0;
    while (string_check >= 0){
        // throw error if line too long
        if (strlen(out)>= MAX_LINE_LENGTH){
            string_check = robust_send_string(fd, "%d %s\r\n", 500, "Line too long");
            continue;
        }
        if (counter == 0){
            printf("%s", out);  // out = 'HELO www.test.com'
            printf("command helper is 0\r\n");
            counter = run_client_initiation (fd , out ,counter, &string_check);
            if (counter == 476) {break;}
            if (counter == 0 ) {
                string_check = robust_nb_read_line(nb, out);
                if (string_check < 0){
                    return;
                }
            }
            continue;
        }
        string_check = robust_nb_read_line(nb, out);
        if (string_check < 0){
            return;
        }
        if (strcasecmp(out, "NOOP\r\n") == 0) {
            size_t checklength = strlen(out);
            if(checklength > 6){
                string_check = robust_send_string(fd, "%d %s\r\n", 500, "Command syntax incorrect for NOOP");
                continue;
            }
            no_operation(fd,&string_check);
            continue;
        }

        if (strcasecmp(out, "QUIT\r\n") == 0) {
            size_t checklength = strlen(out);
            if(checklength > 6){
                string_check = robust_send_string(fd, "%d %s\r\n", 500, "Command syntax incorrect for QUIT");
                continue;
            }
            quit(fd, &string_check);
            printf("Quitting \n");
            free(mail_data);
            nb_destroy(nb);
            destroy_user_list(forward_path);
            close(fd);

            break;
        }
        if (strncasecmp(out, "HELO ", 5) == 0) {
            printf("RESET \n");
            nb_destroy(nb);
            destroy_user_list(forward_path);
            free(mail_data);
            free(reverse_path);
            nb_create(fd, MAX_LINE_LENGTH);
            forward_path = create_user_list();
            counter = 0;
            continue;
        }
        if (strncasecmp(out, "MAIL FROM:", 10) == 0) {
            reverse_path = run_mail_transaction(fd, out, reverse_path,&string_check);
            free(mail_data);
            mail_data = NULL;
            continue;
        }
        if(reverse_path == NULL) {
            string_check = robust_send_string(fd, "%d %s\r\n", 503
                    , "Bad sequence of commands: mail from not specified, should be MAIL FROM:<> or QUIT");
            continue;
        }
        if(mail_data != NULL){
            string_check = robust_send_string(fd, "%d %s\r\n", 503,
                                              "Bad sequence of commands: DATA specified please QUIT or MAIL FROM");
            continue;
        }
        if (strncasecmp(out, "RCPT TO:", 8) == 0) {
            run_receipt_specification(fd,out,&forward_path,&string_check);
            continue;
        }
        if(forward_path == NULL) {
            string_check = robust_send_string(fd, "%d %s\r\n", 503,
                                              "Bad sequence of commands: rcpt to not specified, should be RCPT TO:<>");
            continue;
        }


        if (strcasecmp(out, "DATA\r\n") == 0){
            mail_data = find_message_contents(fd, out , nb, mail_data,&string_check);

            mp *message_param  = malloc(sizeof(mp));
            message_param->message = NULL;
            message_param->message_size = 0;

            message_param = handle_make_message(mail_data, reverse_path,forward_path,message_param);

            char template[] = "temp_fileXXXXXX";
            rn = write_to_file(message_param, template);
            const char* ptr = template;
            save_user_mail(ptr,forward_path);

            printf("FREEING MEMORY \n");

            free(message_param->message);
            message_param->message = NULL;
            message_param->message_size = 0;
            free(message_param);

            free(reverse_path);
            reverse_path = NULL;

            nb_destroy(nb);
            nb_create(fd, MAX_LINE_LENGTH);

            destroy_user_list(forward_path);
            forward_path = NULL;
            forward_path = create_user_list();

            unlink(template);
            close(rn);
            counter++;
        } else {
            string_check = robust_send_string(fd, "%d %s\r\n", 503,
                                              "Bad sequence of commands : OR incorrect syntax, should be DATA");
            continue;
        }
    }
}


