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
        client_initiation.c
        client_initiation.h
        mailuser.c
        mailuser.h
        mysmtpd.c
        netbuffer.c
        netbuffer.h
        server.c
        server.h
        session_initiation.h
        session_initiation.c
        mail_transaction.c
        mail_transaction.h
        receipt_specification.c
        receipt_specification.h
        message_contents.c
        message_contents.h)

# Add a target named `mypopd`
# Followed by a list of source files used to compile the target
add_executable(mypopd
        mailuser.c
        mailuser.h
        mypopd.c
        netbuffer.c
        netbuffer.h
        server.c
        server.h
        transaction_state.c
        transaction_state.h
        update_state.c
        update_state.h)