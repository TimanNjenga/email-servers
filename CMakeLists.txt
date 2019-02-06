cmake_minimum_required(VERSION 3.10)

# Project `Name` and `Language`
project(a3 C)

# Set the language standard to `gnu99`
set(CMAKE_C_STANDARD 99)

# Set the compiler flags
# These flags match those in the makefile
# You should not change them
set(CMAKE_C_FLAGS "-g -Wall -std=gnu99")

# Tell the compiler where to search for include files
# This line might not be necessary
# include_directories(.)

# Add a target named `mysmtpd`
# Followed by a list of source files used to compile the target
add_executable(mysmtpd
        smtp/client_initiation.c
        smtp/client_initiation.h
        mailuser.c
        mailuser.h
        mysmtpd.c
        netbuffer.c
        netbuffer.h
        server.c
        server.h
        smtp/session_initiation.h
        smtp/session_initiation.c
        smtp/mail_transaction.c
        smtp/mail_transaction.h
        smtp/receipt_specification.c
        smtp/receipt_specification.h
        smtp/message_contents.c
        smtp/message_contents.h)

# Add a target named `mypopd`
# Followed by a list of source files used to compile the target
add_executable(mypopd
        mailuser.c
        mailuser.h
        pop3/mypopd.c
        netbuffer.c
        netbuffer.h
        server.c
        server.h
        pop3/transaction_state.c
        pop3/transaction_state.h
        pop3/update_state.c
        pop3/update_state.h)