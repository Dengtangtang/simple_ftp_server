#include "common.h"

static const char *username_lst[] = { "anonymous", "ftp", };

/* handle USER cmd */
void handle_cmd_USER(char *user_arg, Status *status) {
	if (search(user_arg, username_lst, sizeof(username_lst)/sizeof(char *)) == 0) {
		status->is_username_valid = 1;
		set_msg(status->username, user_arg);
		set_msg(status->message, "331 Please input your password\n");
	} else {
		set_msg(status->message, "530 Invalid username\n");
	}

	write_msg(status->conn, status->message);
}

/* handle PASS cmd */
void handle_cmd_PASS(char *pass_arg, Status *status) {
	if ((status->is_username_valid) == 1) {
		status->is_logged_in = 1;
		set_msg(status->message, "230 User successfully logged in.\r\n");
	} else {
		set_msg(status->message, "530 Username or password is invalid.\r\n");
	}
	
	write_msg(status->conn, status->message);
}

/* handle QUIT cmd 
   QUIT parameters are usually prohibited
*/
void handle_cmd_QUIT(Status *status) {
	if (status->is_logged_in) {
		set_msg(status->message, "221 Bye.\r\n");
		write_msg(status->conn, status->message);
		close(status->conn);
		// exit (child) process
		exit(0);
	} else {
		perror("530 User is not logged in");
	}
}

/* handle SYST cmd */
void handle_cmd_SYST(Status *status) {
	if (status->is_logged_in) {
		set_msg(status->message, "215 UNIX Type: L8\r\n");
	} else {
		perror("530 User is not logged in");
	}

	write_msg(status->conn, status->message);
}

/* handle TYPE cmd */
void handle_cmd_TYPE(char *type_arg, Status *status) {
	if (status->is_logged_in) {
		if (type_arg[0] == 'I')
			set_msg(status->message, "200 Type set to I.\r\n");
		else if (type_arg[0] == 'A')
			set_msg(status->message, "200 Type set to A.\r\n");
		else
			set_msg(status->message, "504 Command not implemented for that parameter.\r\n");
	} else {
		perror("530 User is not logged in");
	}

	write_msg(status->conn, status->message);
}

/* handle PASS cmd */
void handle_cmd_PASV(Status *status) {
	
	Port *port_ptr = (Port *)malloc(sizeof(Port));
	int server_ip_addr[4];
	int server_port;
	char bufout[MAXBUF];

	if (status->is_logged_in) {
		gen_port(port_ptr);
		server_port = (port_ptr->p1) * 256 + port_ptr->p2;
		get_ipaddr(status->conn, server_ip_addr);
		sprintf(bufout, "%d.%d.%d.%d.%d.%d", server_ip_addr[0], server_ip_addr[1], server_ip_addr[2], server_ip_addr[3],
				port_ptr->p1, port_ptr->p2);

		close(status->pasv_sk);
		status->pasv_sk = create_socket(server_port);
		status->mode = PASSIVE;

		set_msg(status->message, "227 Entering Passive Mode (");
		cat_msg(status->message, bufout);
		cat_msg(status->message, ")\r\n");
	} else {
		set_msg(status->message, "530 User is not logged in\r\n");
		perror("530 User is not logged in");
	}

	write_msg(status->conn, status->message);
	free(port_ptr);
}

/* handle PORT cmd */
void handle_cmd_PORT(char *port_arg, Status *status) {
	Port *port_ptr = (Port *)malloc(sizeof(Port));
	int client_ip_addr[4];
	int client_port;

	if (status->is_logged_in) {
		int scan_item = sscanf(port_arg, "%d,%d,%d,%d,%d,%d", 
			&client_ip_addr[0], &client_ip_addr[1], &client_ip_addr[2], &client_ip_addr[3],
			&(port_ptr->p1), &(port_ptr->p2));
		// printf("%s%d\n", "scan_item ", scan_item);
		if (scan_item == 6) {
			// set mode to ACTIVE
			status->mode = ACTIVE;
			client_port = (port_ptr->p1) * 256 + (port_ptr->p2);
			// set hostname to client ip addr
			sprintf(status->hostname,"%d.%d.%d.%d", client_ip_addr[0], client_ip_addr[1], client_ip_addr[2], client_ip_addr[3]);
			// set port_num
			status->port_num = client_port;

			set_msg(status->message, "200 PORT mode is setup successfully.\r\n");
		} else {
			status->mode = NORMAL;
			set_msg(status->message, "550 Failed to parse the ip address\r\n");
		}
	} else {
		set_msg(status->message, "530 User is not logged in\r\n");
		perror("530 User is not logged in");
	}

	free(port_ptr);
	write_msg(status->conn, status->message);
}

/* handle RETR cmd 
   The RETR parameter is an encoded pathname of the file
*/
void handle_cmd_RETR(char *retr_arg, Status *status) {
	char bufin[MSGSIZE]; // for retr_arg
	char bufout[MAXBUF];
	int fd, data_conn;
	int read_bytes = 0;
	// int sent_bytes = 0;

	if (status->is_logged_in) {
		// PORT(ACTIVE) mode
		if (status->mode == ACTIVE) {
			// printf("%s\n", "start download in PORT mode.");
			set_msg(bufin, retr_arg);
			fd = open(bufin, O_RDONLY);
			if (access(bufin, R_OK) == 0 && (fd >= 0)) {
				// connect to client (data connection), get data connection socket for server
				status->port_sk = connect_socket(status->hostname, status->port_num);
				// printf("%s%d\n", "data connection is setup, the socket is ", status->port_sk);
				// printf("%d\n", status->conn);
				if (status->port_sk < 0) {
					set_msg(status->message, "425 No tcp connection was established.\r\n");
					write_msg(status->conn, status->message);
					perror("Problem accepting for data connection.");
					return;
				}

				// reply with mask 150
				set_msg(status->message, "150 opening data connection\r\n");
				write_msg(status->conn, status->message);

				// send data to client
				memset(bufout, 0, MAXBUF);
				while ((read_bytes = read(fd, bufout, MAXBUF)) > 0) {
					write(status->port_sk, bufout, read_bytes);
					// printf("%d%s\n", read_bytes, " has been read into socket");
					memset(bufout, 0, MAXBUF);
				}

				// printf("%s\n", "read end");
				set_msg(status->message, "226 File transferred successfully.\r\n");
				// write_msg(status->port_sk, status->message);
				// printf("%s\n", "message sent.");
				write_msg(status->conn, status->message);
				// printf("%s\n", "message sent.");

				close(status->port_sk);
				close(fd);
				return;
			} else {
				set_msg(status->message, "550 Permission denied.\r\n");
			}			
		
		} else if (status->mode == PASSIVE) {
			// printf("%s\n", "start download in PASSIVE mode.");
			set_msg(bufin, retr_arg);
			fd = open(bufin, O_RDONLY);
			if (access(bufin, R_OK) == 0 && (fd >= 0)) {
				data_conn = accept_conn(status->pasv_sk);

				if (data_conn < 0) {
					set_msg(status->message, "425 No tcp connection was established.\r\n");
					write_msg(status->conn, status->message);
					perror("Problem accepting for data connection.");
					return;
				}

				close(status->pasv_sk);
				// reply with mask 150
				set_msg(status->message, "150 opening data connection\r\n");
				write_msg(status->conn, status->message);

				// send data to client
				memset(bufout, 0, MAXBUF);
				while ((read_bytes = read(fd, bufout, MAXBUF)) > 0) {
					write(status->port_sk, bufout, read_bytes);
					memset(bufout, 0, MAXBUF);
				}

				set_msg(status->message, "226 File transferred successfully.\r\n");
				write_msg(status->conn, status->message);

				close(status->port_sk);
				close(fd);
				return;

			} else {
				set_msg(status->message, "550 Permission denied.\r\n");
			}

		} else {
			set_msg(status->message, "550 Please switch mode to PASV.\r\n");
		}
	
	} else {
		set_msg(status->message, "530 User is not logged in\r\n");
		perror("530 User is not logged in");
	}

	write_msg(status->conn, status->message);
	return;
}



