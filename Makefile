CC=gcc
CFLAGS=-g -Wall -std=gnu99

all: mysmtpd mypopd

mysmtpd: mysmtpd.o netbuffer.o mailuser.o server.o client_initiation.o session_initiation.o mail_transaction.o receipt_specification.o message_contents.o
mypopd: mypopd.o netbuffer.o mailuser.o server.o transaction_state.o update_state.o

mysmtpd.o: mysmtpd.c netbuffer.h mailuser.h server.h client_initiation.h session_initiation.h mail_transaction.h receipt_specification.h message_contents.h
mypopd.o: mypopd.c netbuffer.h mailuser.h server.h transaction_state.h update_state.h

netbuffer.o: netbuffer.c netbuffer.h
mailuser.o: mailuser.c mailuser.h
server.o: server.c server.h
client_initiation.o: client_initiation.c client_initiation.h
session_initiation.o: session_initiation.h session_initiation.c
mail_transaction.o: mail_transaction.c mail_transaction.h
receipt_specification.o: receipt_specification.c receipt_specification.h
message_contents.o: message_contents.c message_contents.h
transaction_state.o: transaction_state.c transaction_state.h
update_state.o: update_state.c update_state.h


clean:
	-rm -rf mysmtpd mypopd mysmtpd.o mypopd.o netbuffer.o mailuser.o server.o client_initiation.o session_initiation.o mail_transaction.o receipt_specification.o message_contents.o transaction_state.o update_state.o
cleanall: clean
	-rm -rf *~
