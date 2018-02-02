#include "common.h"
#include "client.h"

int main(int argc, char *argv[])
{
    FILE *log_file;
    int ID = -1, auth = 0, Q = 0;
    struct sockaddr_in serv_addr, from_addr;
    char buffer[BUFLEN], current_cmd[BUFLEN], file_name[BUFLEN];
    int sockfd_atm, sockfd_unlock, server_len, bytes_recieved, i;

    /* Multimea descriptorilor de citire folosita de select() */
    int fdmax;
    fd_set read_fds;
    fd_set tmp_fds; 

    /* Mesaje pentru comunicarea pe TCP */
    msg msg_to_send, msg_recv;
    /* Mesaje pentru comunicarea UDP */
    msg UDP_to_send, UDP_recv;

    /* Verificare parametrii */
    DIE(argc < 3, "Usage ./client IP_server port_server");

    /* Procesare nume si deschidere logfile */
    sprintf(file_name, "client%d.log", getpid());
    printf("File Name: %s\n", file_name);

    log_file = fopen(file_name, "w");

    /* Initializare read_fds si tmp_fds */
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    /* Deschidere socket ATM */
    DIE((sockfd_atm = socket(AF_INET, SOCK_STREAM, 0)) < 0, "socket");

    /* Deschidere socket UNLOCK */
    DIE((sockfd_unlock = socket(AF_INET, SOCK_DGRAM, 0)) < 0, "socket");

    /* Adaugare socket ATM */
    /* Adaugare socket UNLOCK in multimea read_fds */
    FD_SET(0, &read_fds);
    FD_SET(sockfd_atm, &read_fds);
    FD_SET(sockfd_unlock, &read_fds);
    fdmax = sockfd_atm < sockfd_unlock ? sockfd_unlock : sockfd_atm;

    /* Setare struct sockaddr_in pentru a specifica unde trimit datele */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &serv_addr.sin_addr);
    
    /* Conectare ATM */   
    DIE(connect(sockfd_atm,
        (struct sockaddr*) &serv_addr,
        sizeof(serv_addr)) < 0,
    "connect");    

    /* Multiplexare descriptori de citire */
    while(1) {
        tmp_fds = read_fds; 

        /* Verificam pe ce descriptori s-au primit date */
        DIE(select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1,
            "select");

        for(i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {

                /* Daca s-au primit date pe socket-ul TCP */
                if (i == sockfd_atm) {
                    DIE(recv(sockfd_atm, &msg_recv, BUFLEN, 0) < 0, "recv");

                    /* Daca server-ul a anuntat ca se inchide 
                        clientul executa un quit implicit */
                    if (msg_recv.type == QUIT_T) {
                        printf("Server shuted down..\nGoodbye!\n");
                        fprintf(log_file, "Server shuted down..\nimplicit quit\n");
                        Q = 1;
                        break;
                    }

                    /* Setez starea utilizatorului in functie de raspunsul
                        server-ului (detalii in README) */
                    if (msg_recv.type == LOGIN_T) {
                        auth = 1;
                        ID = msg_recv.ID;
                    } else if (msg_recv.type == ERROR_5) {
                        ID = msg_recv.ID;
                    } else if (msg_recv.type == LOGOUT_T) {
                        auth = 0;
                        ID = -1;
                    }

                    /* Afisez la consola mesajul primit */
                    printf("%s\n", msg_recv.payload);

                    /* Scriu in fisier comanda si mesajul primit */
                    fprintf(log_file, "%s%s\n", current_cmd, msg_recv.payload);

                /* Daca s-au primit date pe socket-ul UDP */
                } else if (i == sockfd_unlock) {
                    server_len = sizeof(from_addr);
                    DIE((bytes_recieved = recvfrom(sockfd_unlock,
                        &UDP_recv,
                        BUFLEN,
                        0,
                        (struct sockaddr *) &from_addr,
                        &server_len)) < 0, "recvfrom");

                    if (bytes_recieved > 0) {

                        /* Afisez la consola mesajul primit */
                        printf("%s\n", UDP_recv.payload);

                        /* Scriu in logfile comanda si mesajul primit */
                        fprintf(log_file, "%s%s\n", current_cmd, UDP_recv.payload);

                        /* Daca server-ul a cerut parola secreta */
                        if (UDP_recv.type == GIVE_PASS) {
                            memset(buffer, 0 , BUFLEN);
                            fgets(buffer, BUFLEN-1, stdin);

                            /* Salvez parola pentru a o scrie in logfile */
                            strcpy(current_cmd, buffer);

                            /* Formez mesajul cu parola */
                            make_UDP_msg(&UDP_to_send, PASS, ID, buffer);

                            /* Trimit mesajul server-ului pe UDP */
                            DIE(sendto(sockfd_unlock,
                                &UDP_to_send,
                                UDP_to_send.msg_len,
                                0,
                                (struct sockaddr *) &serv_addr,
                                sizeof(serv_addr)) < 0, "sendto");
                        }
                    }
                } else {
                    /* Citesc comanda de la tastatura */
                    memset(buffer, 0 , BUFLEN);
                    fgets(buffer, BUFLEN-1, stdin);

                    /* Salvez comanda pentru a o scrie ulterior in logfile */
                    strcpy(current_cmd, buffer);

                    /* Traduc comanda in mesaj pentru server */
                    get_msg_for_command(&msg_to_send, buffer);

                    /* Verificam erorile tratate de client side */
                    if (!client_command_check(&msg_to_send, auth)) {
                        printf("%s\n", msg_to_send.payload);
                        fprintf(log_file, "%s%s\n", current_cmd, msg_to_send.payload);
                        continue;
                    }

                    /* Daca s-a apelat unlock, initiez comunicarea pe UDP */
                    if (msg_to_send.type == UNLOCK_T) {
                        make_UDP_msg(&UDP_to_send, UNLOCK_T, ID, "-");

                        DIE(sendto(sockfd_unlock,
                            &UDP_to_send,
                            UDP_to_send.msg_len,
                            0,
                            (struct sockaddr *) &serv_addr,
                            sizeof(serv_addr)) < 0, "sendto");
                        continue;
                    }

                    /* Verificam daca utilizatorul a apelat quit */
                    if (msg_to_send.type == QUIT_T) {
                        printf("Goodbye!\n");
                        fprintf(log_file, "%s", current_cmd);

                        make_msg(&msg_to_send, QUIT_T);

                        msg_to_send.ID = ID;
                        DIE(send(sockfd_atm,
                            &msg_to_send,
                            msg_to_send.msg_len, 0) < 0, "send");
                        Q = 1;
                        break;
                    }

                    /* Trimit mesajul la server pe TCP */
                    msg_to_send.ID = ID;
                    DIE(send(sockfd_atm,
                        &msg_to_send,
                        msg_to_send.msg_len, 0) < 0, "send");
                }
            }
        }

        /* S-a primit quit */
        if (Q == 1)
            break;
    }

    /* Inchidere socket TCP */
    DIE(close(sockfd_atm) < 0, "close");

    /* Inchidere socket UDP */
    DIE(close(sockfd_unlock) < 0, "close");

    /* Inchidere logfile */
    fclose(log_file);

    return EXIT_SUCCESS;
}
