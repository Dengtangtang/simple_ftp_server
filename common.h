#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>     /* defines STDIN_FILENO, system calls,etc */
#include <sys/types.h>  /* system data type definitions, getsockopt */
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/socket.h> /* socket specific definitions */
#include <netinet/in.h> /* INET constants and stuff */
#include <arpa/inet.h>  /* IP address conversion stuff */
#include <netdb.h>      /* gethostbyname */
#include <string.h>


#define MAXBUF 1024
#define MSGSIZE 256
#define NAMESIZE 32

/* define a enum type for a list of commands */
typedef enum Cmd_Lst { USER, PASS, PORT, QUIT, RETR, SYST, TYPE } Cmd_lst;

/* enum for mode */
typedef enum Conn_Mode { NORMAL, PASSIVE, ACTIVE } Conn_mode;

typedef struct Command {
	char command[5];
	char arg[MAXBUF];
} Command;

typedef struct Status {
	// connection fd
	int conn;

	// connection state: NORMAL, PASV(PASSIVE), PORT(ACTIVE)
	int mode;

	// is username valid
	int is_username_valid;

	// is logged in
	int is_logged_in;

	/* Record the current username */
	char username[NAMESIZE];

	// Messages response to client
	char message[MSGSIZE];

	/* root directory */
	char root[MSGSIZE];

	/* current directory */
	char curdir[MSGSIZE];

	/* old path for rename */
	char oldpath[MSGSIZE];

	// the name of host (ip addr)
	char hostname[NAMESIZE];

	/* the name of log file */
	char logname[NAMESIZE];

	// pasv mode socket
	int pasv_sk;

	// port mode socket
	int port_sk;

	/* (client) port used in Port mode */
	int port_num;

} Status;

typedef struct Port {
	int p1;
	int p2;
} Port;

/* server functions */
int create_socket(int);
void launch_server(int);
void set_msg(char *, char *);
void write_msg(int, char *);
void cat_msg(char *, char *);
int search(char *, const char **, int);
void handle_cmds(Command *, Status *);
int connect_socket(char *, int);


/* commands handling functions */
void handle_cmd_USER(char *, Status *);
void handle_cmd_PASS(char *, Status *);
void handle_cmd_QUIT(Status *);
void handle_cmd_SYST(Status *);
void handle_cmd_TYPE(char *, Status *);
void handle_cmd_PORT(char *, Status *);
void handle_cmd_RETR(char *, Status *);


#endif