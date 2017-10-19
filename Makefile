server: handle_cmds.c server.c
	gcc handle_cmds.c server.c -Wall -o server
clean:
	-rm server