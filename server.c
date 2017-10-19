#include "common.h"

/* command list 
*/
static const char *str_cmd_lst[] = {
	"USER", "PASS", "PORT", "QUIT", "RETR", "SYST", "TYPE",
};


/* set msg by cpy value
   @para msg(destination)
   @para value(src)
*/
void set_msg(char *msg, char *value) {
	memset(msg, 0, strlen(msg));
	strcpy(msg, value);
}

/* write msg to specified file descriptor (client)
   @para fd, file descriptor
   @para msg, written message
*/
void write_msg(int fd, char *msg) {
	// msg will be written into fd
	write(fd, msg, strlen(msg));
}

/* append src to destination
*/
void cat_msg(char *dest, char *src) {
	strcat(dest, src);
}

/* search cmd in str_cmd_lst (actually a pointer array)
   @para target string
   @para string array to be searched for
   @para length of string array
   @return fail -1; success index of command in the array
*/
int search(char *str, const char **str_lst, int str_len) {
	int i;
	for (i = 0; i < str_len; ++i) {
		if (strcmp(str, str_lst[i]) == 0)
			return i;
	}
	return -1;
}

/* handle different commands from client
   @para cmd
   @para status
*/
void handle_cmds(Command *cmd, Status *status) {
	switch(search(cmd->command, str_cmd_lst, sizeof(str_cmd_lst)/sizeof(char *))) {
		case USER:
			handle_cmd_USER(cmd->arg, status); break;
		case PASS:
			handle_cmd_PASS(cmd->arg, status); break;
		case QUIT:
			handle_cmd_QUIT(status); break;
		case SYST:
			handle_cmd_SYST(status); break;
		case TYPE:
			handle_cmd_TYPE(cmd->arg, status); break;
		case PORT:
			handle_cmd_PORT(cmd->arg, status); break;
		case RETR:
			handle_cmd_RETR(cmd->arg, status); break;
		default:
			set_msg(status->message, "500 Unknown commands\n");
			write_msg(status->conn, status->message);
			break;
	}
}

int connect_socket(char *hostname, int port) {
	int sk;
	struct sockaddr_in peer_skaddr;
	struct hostent *hp;

	sk = socket(PF_INET, SOCK_STREAM, 0);
	if (sk < 0) {
		printf("%s\n", "Problem on creating data connection socket!");
		return -1;
	}

	peer_skaddr.sin_family = AF_INET;
	hp = gethostbyname(hostname);
	memcpy(&(peer_skaddr.sin_addr.s_addr), hp->h_addr, hp->h_length);
	peer_skaddr.sin_port = htons(port);

	if (connect(sk, (struct sockaddr *)&peer_skaddr, sizeof(peer_skaddr)) == 0) {
		return sk;
	} else {
		printf("Failure on data connection!\n");
		return -1;
	}

}


/* creates a socket on specified port number 
   @param port number
   @return file descriptor for created socket
*/

int create_socket(int port) {
	int sk;
	struct sockaddr_in skaddr;
	int reuse = 1;

	// tcp use SOCK_STREAM
	sk = socket(PF_INET, SOCK_STREAM, 0);
	if (sk < 0) {
		perror("Problem on creating socket!");
	}

	// set up server
	skaddr.sin_family = AF_INET;
	skaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	skaddr.sin_port = htons(port);

	/* Address can be reused instantly after program exits,
	   it shall be called before bind, otherwise no effects
	*/
	setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	// assign name to created socket
	if (bind(sk, (struct sockaddr*) &skaddr, sizeof(skaddr)) < 0) {
		perror("Problem binding");
	}

	/* int listen(int sock, int backlog)
	   makes sock(descriptor) into a passive socket for connection, later it will use accept()
	   backlog defines the maximun length of request queue
	*/
	listen(sk, 5);
	return sk;
}

void launch_server(int port) {
	int sk = create_socket(port);
	struct sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int conn, pid, read_bytes;
	char bufin[MAXBUF];

	while (1) {
		// create a new file descriptor for accepted socket -- conn here. (origin socket sk is still open for listening)
		conn = accept(sk, (struct sockaddr*) &(client_addr), &client_addr_len);
		if (conn < 0) {
			//printf("%d\n", conn);
			perror("Problem accepting");
			continue;
		}

		/* create child process by fork() to support multi-client,
		   parent process can still sniff whether there are still request
		*/
		pid = fork();

		// clean
		memset(bufin, 0, MAXBUF);

		if (pid == 0) {
			close(sk);
			printf("%s\n", "The ftp server is ready.");

			// allocate memory
			Command *cmd = (Command *)malloc(sizeof(Command));
			Status *status = (Status *)malloc(sizeof(Status));

			status->conn = conn;
			set_msg(status->message, "220 Welcome to Dengtangtang ftp server.\r\n");
			write_msg(conn, status->message);
			//printf("%s", status->message);

			/* read commands from client,
			   
			   ssize_t read(int fd, void *buf, size_t count),
			   read() attempts to read up to count bytes from file descriptor fd
			   into the buffer starting at buf.
			   On success, the number of bytes read is returned (zero indicates end
			   of file), and the file position is advanced by this number.
			*/

			while ((read_bytes = read(conn, bufin, MAXBUF))) {
				
				if (read_bytes <= MAXBUF) {
					// read from bufin and assign them to struct Command
					sscanf(bufin,"%s %s",cmd->command, cmd->arg);

					// handle commands from client
					handle_cmds(cmd, status);

					// clear buffer and cmd memory block
					memset(bufin, 0, MAXBUF);
					memset(cmd, 0, sizeof(*cmd));

				} else {
					perror("Problem reading.");
				}
			}

			// free memory
			free(cmd);
			free(status);

		} else if (pid < 0) {
			printf("%s\n", "FTP server cannot create child process.");
			exit(1);
		} else {
			printf("%s\n", "Parent process is running...");

			// close accepted socket
			close(conn);
		}

	}
}

int main(int argc, char* argv[]) {
	int port = 9897;
	
	// if(argc > 2)
	// 	port = atoi(argv[2]);

	launch_server(port);

	return 0;
}