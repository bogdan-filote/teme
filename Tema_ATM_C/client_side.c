#include "common.h"
#include "client.h"

/* Functie ce trateaza comanda introdusa de utilizator
    de la tastatura si formeaza mesajul corespunzator
    pentru comunicarea cu server-ul pe socket-ul TCP */
void get_msg_for_command(msg *msg_to_send, char *buffer)
{
    char *cmd;
    const char delim[3] = " \n";

    cmd = strtok(buffer, delim);

    if (strcmp(cmd, LOGIN) == 0)
        make_msg_args(msg_to_send, LOGIN_T, delim);

    else if (strcmp(cmd, LOGOUT) == 0)
        make_msg(msg_to_send, LOGOUT_T);

    else if (strcmp(cmd, LIST_SOLD) == 0)
        make_msg(msg_to_send, LIST_SOLD_T);

    else if (strcmp(cmd, GET_MONEY) == 0)
        make_msg_args(msg_to_send, GET_MONEY_T, delim);

    else if (strcmp(cmd, PUT_MONEY) == 0)
        make_msg_args(msg_to_send, PUT_MONEY_T, delim);

    else if (strcmp(cmd, UNLOCK) == 0)
        make_msg(msg_to_send, UNLOCK_T);

    else if (strcmp(cmd, QUIT) == 0)
        make_msg(msg_to_send, QUIT_T);

    else
        msg_to_send->type = INPUT_ERROR;
}

/* Trateaza erorile ce apar la client_side
    si formeaza mesajul corespunzator care
    va fi afisat la consola si scris in logfile,
    dar nu va fi trimis server-ului */
int client_command_check(msg *msg_to_send, int auth)
{
    if (msg_to_send->type == INPUT_ERROR) {
        make_error_msg(msg_to_send, INPUT_ERROR, IN_ERR_MSG);
        return 0;
    }

    if (msg_to_send->type == LOGIN_T && auth == 1) {
        make_error_msg(msg_to_send, ERROR_2, OPENED_SESSION);
        return 0;
    }

    if ((msg_to_send->type == LOGOUT_T
        || msg_to_send->type == LIST_SOLD_T
        || msg_to_send->type == GET_MONEY_T
        || msg_to_send->type == PUT_MONEY_T)
        && auth == 0) {
        make_error_msg(msg_to_send, ERROR_1, UNAUTH);
        return 0;
    }
    return 1;
}

void make_error_msg(msg *msg_to_send, int errno, char *err_msg)
{
    sprintf(msg_to_send->payload,
        "%d : %s", errno, err_msg);
}

void make_msg(msg *msg_to_send, int type)
{
    msg_to_send->type = type;
    sprintf(msg_to_send->payload, "-");
    msg_to_send->msg_len = strlen(msg_to_send->payload) + 3 * sizeof(int);
}

void make_msg_args(msg *msg_to_send, int type, const char *delim)
{
    char *token;

    msg_to_send->type = type;
    sprintf(msg_to_send->payload,
        "%s", strtok(NULL, delim));

    if ((token = strtok(NULL, delim)) != NULL)
        sprintf(msg_to_send->payload,
            "%s %s", msg_to_send->payload, token);

    msg_to_send->msg_len = strlen(msg_to_send->payload) + 3 * sizeof(int);
}

void make_UDP_msg(msg *UDP_to_send, int type, int ID, char *buffer)
{
    UDP_to_send->type = type;
    UDP_to_send->ID = ID;
    sprintf(UDP_to_send->payload, "%s", buffer);
    UDP_to_send->msg_len = strlen(UDP_to_send->payload) + 3 * sizeof(int);
}
