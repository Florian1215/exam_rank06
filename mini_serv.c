/* ----------------- INCLUDE ----------------- */
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>


/* ---------------- PROTOTYPE ---------------- */
void    exit_with_message(const char *message);
void    fatal_error();
void	emit(int exclude_client);
void	clear_buffer(int fd);

void	main_loop();
void	select_fd_event();
void	connect_new_client();
void	handle_client_event(int fd);
void	handle_client_message(int fd, int msg_len);
void	disconnect_client(int fd);


/* ----------------- STRUCT ------------------ */
struct s_clients
{
	int     id;
	char    buffer[280000];
};


/* ----------------- TYPEDEF ----------------- */
typedef struct s_clients    t_clients;
typedef struct sockaddr_in	t_sockaddr_in;


/* ------------- GLOBAL VARIABLE ------------- */
t_clients   clients[2049];
char        read_buffer[300000];
char        send_buffer[300000];
fd_set		read_set;
fd_set		write_set;
fd_set		save_set;
int         max_fd;
int         socket_fd;
int         global_id;

char		ARGS_ERROR[] = "Wrong number of arguments\n";
char		FATAL_ERROR[] = "Fatal error\n";
char		CLIENT_MESSAGE[] = "client %d: %s\n";
char		NEW_CLIENT[] = "server: client %d just arrived\n";
char		DISCONNECT_CLIENT[] = "server: client %d just left\n";


/* ------------------ MAIN ------------------- */
int main(const int argc, char **argv)
{
	t_sockaddr_in	serv_addr;

	if (argc != 2)
		exit_with_message(ARGS_ERROR);

	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd == -1)
		fatal_error();

	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(2130706433);
	serv_addr.sin_port = htons(atoi(argv[1]));

	global_id = 0;
	max_fd = socket_fd;

	bzero(clients, sizeof(clients));

	FD_ZERO(&read_set);
	FD_ZERO(&write_set);
	FD_ZERO(&save_set);
	FD_SET(socket_fd, &save_set);

	if (bind(socket_fd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0)
		fatal_error();

	if (listen(socket_fd, 100) != 0)
		fatal_error();

	main_loop();
}

void	main_loop()
{
	while (1)
	{
		read_set = write_set = save_set;

		if (select(max_fd + 1, &read_set, &write_set, 0, 0) == -1)
			fatal_error();
		select_fd_event();
	}
}

void	select_fd_event()
{
	int fd = 0;

	while (fd <= max_fd)
	{
		if (FD_ISSET(fd, &read_set))
		{
			if (fd == socket_fd)
			{
				connect_new_client();
				break ;
			}
			const int	len_msg = (int)recv(fd, read_buffer, sizeof(read_buffer), 0);

			if (len_msg <= 0)
			{
				disconnect_client(fd);
				break ;
			}
			handle_client_message(fd, len_msg);
		}
		fd++;
	}
}

void	connect_new_client()
{
	t_sockaddr_in		cli;
	socklen_t			len = sizeof(cli);
	const int			fd = accept(socket_fd, (struct sockaddr *)&cli, &len);

	if (fd == -1)
		fatal_error();
	if (fd > max_fd)
		max_fd = fd;
	clients[fd].id = global_id++;
	FD_SET(fd, &save_set);
	sprintf(send_buffer, NEW_CLIENT, clients[fd].id);
	emit(fd);
}

void	disconnect_client(const int fd)
{
	sprintf(send_buffer, DISCONNECT_CLIENT, clients[fd].id);
	emit(fd);
	FD_CLR(fd, &save_set);
	close(fd);
	clear_buffer(fd);
}

void	handle_client_message(const int fd, const int msg_len)
{
	int	i = 0;
	int	j = (int)strlen(clients[fd].buffer);

	while (i < msg_len)
	{
		clients[fd].buffer[j] = read_buffer[i];
		if (clients[fd].buffer[j] == '\n')
		{
			clients[fd].buffer[j] = '\0';
			sprintf(send_buffer, CLIENT_MESSAGE, clients[fd].id, clients[fd].buffer);
			emit(fd);
			clear_buffer(fd);
			j = -1;
		}
		i++;
		j++;
	}
}


/* ------------------ UTILS ------------------ */
void	emit(const int exclude_client)
{
	int			fd = 3;
	const int	buffer_len = (int)strlen(send_buffer);

	while (fd <= max_fd)
	{
		if (FD_ISSET(fd, &write_set) && fd != socket_fd && fd != exclude_client)
		{
			if (send(fd, send_buffer, buffer_len, 0) == -1)
				fatal_error();
		}
		fd++;
	}
}

void    fatal_error()
{
	exit_with_message(FATAL_ERROR);
}

void    exit_with_message(const char *message)
{
	write(2, message, strlen(message));
	exit(1);
}

void	clear_buffer(const int fd)
{
	bzero(clients[fd].buffer, sizeof(clients[fd].buffer));
}
