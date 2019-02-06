#include "../netbuffer.h"
#include "../mailuser.h"
#include "../server.h"
#include "transaction_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 1024




static void handle_client(int fd);

int main(int argc, char *argv[]) {

    if (argc != 2) {
        fprintf(stderr, "Invalid arguments. Expected: %s <port>\n", argv[0]);
        return 1;
    }

    run_server(argv[1], handle_client);

    return 0;
}



/*
 * Session Termination(quit)
 * QUIT
 *
 */
void quit (int fd , int* string_check ) {
    (*string_check) += robust_send_string(fd, "%s\r\n", "+OK POP3 server signing off\r\n");
}


int spaceHelper(const char* out){
    if ((out[4]==' ') && (out[5] != ' ')){
        printf("USER has a space after \r\n");
        return 0;
    } else{
        return 1;
    }
}

char* user_authentication (int fd, char* out, int* string_check) {

    printf("in user_authentication \r\n");

    const char ch = ' ';
    char *user_name ;

    user_name = strchr(out, ch);

    if (user_name == NULL){   // 3 = SPACE + CR + LF
        (*string_check) += robust_send_string(fd, "%s\r\n", "-ERR Command parameter missing");
        return NULL;
    }
    size_t length = strlen(user_name);
    if (length <= 3){         // 3 = SPACE + CR + LF
        (*string_check) += robust_send_string(fd, "%s\r\n","-ERR Command parameter missing");
        return NULL;
    }

    printf("user_name:%s", user_name);        // user_name = " me@me.com"
    user_name += 1;

    char *rightStr;
    rightStr = strstr(user_name,"\r\n");      //find CRLF
    rightStr[0] = 0 ;                         //remove it from user_name

    if ((is_valid_user(user_name, NULL)) > 0){
        (*string_check) += robust_send_string(fd, "+OK user name is valid\r\n");
        return user_name;
    } else {
        (*string_check) += robust_send_string(fd, "-ERR no such user detected\r\n");
        return NULL;
    }
}


int pass_authentication (int fd, char* out , int counter, char* user_name, int* string_check) {

    printf("in pass_authentication \r\n");

    const char ch = ' ';
    char *user_pass;

    user_pass = strchr(out, ch);

    if (user_pass == NULL){
        (*string_check) += robust_send_string(fd, "%s\r\n","-ERR Command parameter missing");
        return 2;
    }

    printf("user_pass:%s", user_pass);// user_pass = " mepassword"
    user_pass += 1;
    if (!user_pass){
        (*string_check) += robust_send_string(fd, "%s\r\n","-ERR Command parameter missing");
        return 2;
    }

    char *thisrightStr;
    thisrightStr = strstr(user_pass,"\r\n");      //find CRLF
    thisrightStr[0] = 0 ;                         //remove it from user_pass


    if ((is_valid_user(user_name, user_pass)) > 0){
        (*string_check) += robust_send_string(fd, "+OK user pass is valid\r\n");
        counter++;
        return counter;
    }
    else {
        (*string_check) += robust_send_string(fd, "-ERR wrong password\r\n");
        counter = 2;
        return counter;
    }
}


void handle_client(int fd) {

    //AUTHORIZATION STATE
    /*
     * USER/PASS (plain authentication) - done above
     * QUIT (session termination) - done above  if the client
     * issues the QUIT command from the AUTHORIZATION state, the POP3
     * session terminates but does NOT enter the UPDATE state.
     */


    char out[MAX_LINE_LENGTH];
    net_buffer_t nb = nb_create(fd, MAX_LINE_LENGTH);
    char* name = NULL;
    int counter = 0;
    int string_check = 0;  // to check that send_string doesnt return -1


    while (string_check == 0){

        if (counter == 0 ) {
            string_check += robust_send_string(fd, "+OK POP3 server ready\r\n");
            counter = 1;
            continue;
        }

        if (counter == 1) {
            if (name != NULL){
                string_check += robust_send_string(fd, "-ERR User name already verified , check password\r\n");
                counter = 2;
                continue;
            }
            string_check += robust_nb_read_line(nb, out);

            if (strcasecmp(out, "QUIT\r\n") == 0) {
                size_t checklength = strlen(out);
                if(checklength > 6){
                    string_check += robust_send_string(fd, "-ERR invalid QUIT command\r\n");
                    continue;
                }
                quit(fd, &string_check);
                break;
            }
            if ((strncasecmp(out, "USER ", 5) == 0) && (spaceHelper(out) ==0)) {
                char* temp = user_authentication(fd, out, &string_check);
                if (temp == NULL) { counter = 1; continue; }
                else { counter = 2; }
                name = (char *) malloc(strlen (temp));
                strcpy(name, temp);
                continue;
            } else {
                string_check += robust_send_string(fd, "%s\r\n", "-ERR invalid USER command ");
                counter = 1;
            }
        }
        if  (counter == 2){
            string_check = robust_nb_read_line(nb, out);
            if (string_check < 0){
                break;
            }
            if (strcasecmp(out, "QUIT\r\n") == 0) {
                size_t checklength = strlen(out);
                if(checklength > 6){
                    string_check += robust_send_string(fd, "-ERR invalid QUIT command\r\n");
                    continue;
                }
                quit(fd, &string_check);
                break;
            }
            if ((strncasecmp(out, "USER", 4) == 0)&& (spaceHelper(out) ==0)){
                counter = 1;
                continue;
            }
            if ((strncasecmp(out, "PASS", 4) == 0)&& (spaceHelper(out) ==0)) {
                counter = pass_authentication(fd, out, counter, name, &string_check);
                continue;
            } else {
                string_check += robust_send_string(fd, "%s\r\n", "-ERR invalid PASS command ");
                if (string_check < 0){
                    break;
                }
            }
        }

        if (counter > 2){
            string_check += robust_nb_read_line(nb, out);
            // ENTER THE TRANSACTION STATE
            run_transaction_state(name, out, fd, nb) ;
            free(name);
            break;
        }
    }
}
