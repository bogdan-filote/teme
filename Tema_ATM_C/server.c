#include "common.h"
#include "server.h"

int main(int argc, char *argv[])
{
	int PORT, MAX_CLIENTS;
	database DB;
	char buffer[BUFLEN], cmd[BUFLEN];
	struct sockaddr_in serv_addr, from_addr, UDP_from_addr;
	int listen_fd_atm, sock_fd_unlock, current_fd, new_client_fd;
	int bytes_recieved, client_len, UDP_from_len, Q = 0;

	/* Mesaje pentru comunicarea pe TCP */
	msg msg_to_send, msg_recv;
	/* Mesaje pentru comunicarea UDP */
	msg UDP_to_send, UDP_recv;

	/* Multimea descriptorilor de citire folosita de select() */
	int fdmax;
	fd_set read_fds;
	fd_set tmp_fds;

	/* Verificare parametrii */
	DIE(argc < 3, "Usage ./server port_server users_data_file");

	/* Parsare users_data_file si inregistrare useri in DB */
	init_database(&DB, argv[2]);

	/* Initializare port si numarul maxim de clienti */
	PORT = atoi(argv[1]);
	MAX_CLIENTS = 2 * DB.number_of_users;

	/* Initializare read_fds si tmp_fds */
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	/* Deschidere socket TCP de ascultare */
	DIE((listen_fd_atm = socket(AF_INET, SOCK_STREAM, 0)) < 0, "socket");

	/* Deschidere socket UDP pentru serviciu de deblocare */
	DIE((sock_fd_unlock = socket(AF_INET, SOCK_DGRAM, 0)) < 0, "socket");

	/* Setare struct sockaddr_in pentru a asculta pe portul respectiv */
	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(PORT);

	/* Legare proprietati de socket TCP de ascultare */ 
	DIE(bind(listen_fd_atm,
  	(struct sockaddr *) &serv_addr,
  	sizeof(struct sockaddr)) < 0, "bind");

	/* Legare proprietati de socket UDP */
	DIE(bind(sock_fd_unlock,
  	(struct sockaddr *) &serv_addr,
  	sizeof(struct sockaddr)) < 0, "bind");

	/* Ascultare de conexiuni TCP (pentru ATM) */
 	DIE(listen(listen_fd_atm, MAX_CLIENTS) < 0, "listen");

	/* Adaugare socket pe care se asculta conexiuni */
	/* Adaugare socket UDP in multimea read_fds */
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(listen_fd_atm, &read_fds);
	FD_SET(sock_fd_unlock, &read_fds);
	fdmax = listen_fd_atm < sock_fd_unlock ? sock_fd_unlock : listen_fd_atm;

	/* Multiplexare descriptori de citire */
	do {
		tmp_fds = read_fds;

		/* Verificam pe ce descriptori s-au primit date */
		DIE(select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) < 0, "select");

		/* Iteram prin multimea descriptorilor de citire */
		for (current_fd = 0; current_fd <= fdmax; current_fd++) {

			/* Daca s-a primit pe current_fd */
			if (FD_ISSET(current_fd, &tmp_fds)) {

				/* Daca s-a primit comanda de la tastatura */
				if (current_fd == STDIN_FILENO) {

					/* Citim comanda */
					memset(buffer, 0 , BUFLEN);
          			fgets(buffer, BUFLEN-1, stdin);

          			/* Eliminam terminatorul de sir */
          			strncpy(cmd, buffer, strlen(buffer) - 1);

          			/* Daca s-a primit quit */
          			if (strcmp(cmd, QUIT) == 0) {
          				if (fdmax >= 5)

          					/* Anunt clentii ca serverul se inchide */
          					/* Le inchid sesiunea */
							for (current_fd = 5; current_fd <= fdmax; current_fd++) {
								msg_to_send.type = QUIT_T;
								msg_to_send.msg_len = 3 * sizeof(int);
								DIE(send(current_fd, &msg_to_send, msg_to_send.msg_len, 0) < 0, "send");
								DIE(close(current_fd) < 0, "close");
							}
          				Q = 1;
          				break;
          			}

				/* Daca s-a primit pe socket-ul de ascultare */
				} else if (current_fd == listen_fd_atm) {

					/* Un client nou a cerut sa se conecteze */
					/* Actiunea serverului: accept() */
					client_len = sizeof(from_addr);
					DIE((new_client_fd = accept(listen_fd_atm,
						(struct sockaddr *) &from_addr,
						&client_len)) < 0, "accept");

					/* Adaugam socket-ul intors de accept() la read_fds */
					FD_SET(new_client_fd, &read_fds);
					if (new_client_fd > fdmax) 
						fdmax = new_client_fd;

					printf("Noua conexiune de la %s, port %d, socket_client %d\n",
						inet_ntoa(from_addr.sin_addr),
						ntohs(from_addr.sin_port),
						new_client_fd);

				/* Daca s-a primit pe socket-ul UDP */
				} else if (current_fd == sock_fd_unlock) {

					/* Citim ce a venit */
					UDP_from_len = sizeof(UDP_from_addr);
					DIE((bytes_recieved = recvfrom(sock_fd_unlock,
						&UDP_recv,
						BUFLEN,
						0,
						(struct sockaddr *) &UDP_from_addr,
						&UDP_from_len)) < 0, "recvfrom");

					/* Procesam raspunul pentru ce a venit */
					if (bytes_recieved > 0)
						get_UDP_response(&UDP_to_send, UDP_recv, &DB);

					/* Trimitem clientului raspunsul */
					DIE(sendto(sock_fd_unlock,
						&UDP_to_send,
						UDP_to_send.msg_len,
						0,
						(struct sockaddr *) &UDP_from_addr,
						sizeof(UDP_from_addr)) < 0, "sendto");

				/* Daca s-a primit ceva pe unul din socketii activi */
				} else {

					/* Citim ce a venit */
					memset(&msg_recv, 0, BUFLEN);
					DIE((bytes_recieved = recv(current_fd,
						&msg_recv,
						BUFLEN,
						0)) < 0, "recv");

					/* Daca "s-au primit 0 bytes" */
					if (bytes_recieved == 0) {

						/* Conexiunea s-a inchis la celalalt capat */
						printf("Client %d hung up\n", current_fd);

						/* O inchidem si aici si o stergem din read_fds */
						DIE(close(current_fd) < 0, "close"); 
						FD_CLR(current_fd, &read_fds);

					/* S-a primit un mesaj > 0 */
					} else {
						printf ("Clientul %d a trimis mesajul tip: %d\n",
							current_fd,
							msg_recv.type);

						/* Procesez mesajul de la client si obtin raspunsul */
						get_answer_for_msg(&msg_to_send, msg_recv, &DB);

						/* Trimit raspunsul clientului */
						DIE(send(current_fd,
							&msg_to_send,
							msg_to_send.msg_len,
							0) < 0,
							"send");
					}
				} 
			}
		}

		/* S-a primit quit */
		if (Q == 1)
			break;
  } while (1);

  /* Eliberare structuri alocate pe heap */
  free(DB.users);

  /* Inchidere socket de ascultare */
  DIE(close(listen_fd_atm) < 0, "close");

  /* Inchidere socket UDP */
  DIE(close(sock_fd_unlock) < 0, "close");
   
  return EXIT_SUCCESS; 
}
