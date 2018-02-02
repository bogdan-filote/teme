#include "common.h"
#include "server.h"

void init_database(database *DB, char *users_data_file)
{
    FILE *fin;
    char line[LINE_LENGTH];
    const char delim[3] = " \n";
    char *token;
    int index = 0, i;

    /* Open users_data_file */
    fin = fopen(users_data_file, "r");
    DIE(fin == NULL, "Open users_data_file");

    /* Get N - number of users */
    fgets(line, LINE_LENGTH, fin);
    DB->number_of_users = atoi(line);

    DIE((DB->users = malloc(DB->number_of_users * sizeof(user))) == NULL,
        "malloc users");

    while (fgets(line, LINE_LENGTH, fin)) {

        /* Setam numele */
        token = strtok(line, delim);
        sprintf(DB->users[index].nume, "%s", token);

        /* Setam prenumele */
        token = strtok(NULL, delim);
        sprintf(DB->users[index].prenume, "%s", token);

        /* Setam numarul cardului */
        token = strtok(NULL, delim);
        sprintf(DB->users[index].numar_card, "%s", token);

        /* Setam pinul */
        token = strtok(NULL, delim);
        sprintf(DB->users[index].pin, "%s", token);

        /* Setam parola secreta si soldul */
        token = strtok(NULL, "\n");
        i = strlen(token);
        while(token[--i] != ' ');

        /* Soldul */
        DB->users[index].sold = atof(token+i);

        /* Parola secreta */
        strncpy(DB->users[index].parola_secreta, token, i);
        DB->users[index].parola_secreta[i] = '\n';

        DB->users[index].tries = 0;
        DB->users[index].auth = 0;
        DB->users[index].blocked = 0;

        index++;
    }

    fclose(fin);
}

void get_answer_for_msg(msg *msg_to_send, msg msg_recv, database *DB)
{
    const char atm[] = "ATM>";
    int index = msg_recv.ID;
    msg_to_send->ID = -1;
    switch (msg_recv.type) {
        /* S-a primit login */
        case LOGIN_T: {
            switch(check_login(msg_recv.payload, DB, &index)) {
                case SUCCESS: {
                    make_welcome_msg(msg_to_send,
                        DB->users[index].nume,
                        DB->users[index].prenume, atm);
                    msg_to_send->ID = index;
                    break;
                }
                case ERROR_2:
                    make_error_msg(msg_to_send, ERROR_2,
                        atm, OPENED_SESSION);
                    break;

                case ERROR_3:
                    make_error_msg(msg_to_send, ERROR_3,
                        atm, WRONG_PIN);
                    break;

                case ERROR_4:
                    make_error_msg(msg_to_send, ERROR_4,
                        atm, UNREG_CARD);
                    break;

                case ERROR_5: {
                    make_error_msg(msg_to_send, ERROR_5,
                        atm, BLOCKED_CARD);
                    msg_to_send->ID = index;
                    break;
                }
                default: break;
            }
            break;
        }
        /* S-a primit logout */
        case LOGOUT_T: {
            DB->users[index].auth = 0;
            make_logout_msg(msg_to_send, atm);
            msg_to_send->ID = index;
            break;
        }
        /* S-a primit listsold */
        case LIST_SOLD_T: {
            msg_to_send->type = LIST_SOLD_T;
            sprintf(msg_to_send->payload,
                "%s %.2f", atm, DB->users[index].sold);
            msg_to_send->ID = index;
            break;
        }
        /* S-a primit getmoney */
        case GET_MONEY_T: {
            switch(check_sold(msg_recv.payload, DB, index)) {
                case SUCCESS: {
                    make_get_money_msg(msg_to_send, atm, atoi(msg_recv.payload));
                    break;
                }
                case ERROR_8:
                    make_error_msg(msg_to_send, ERROR_8,
                        atm, INSUF_FUNDS);
                    break;

                case ERROR_9:
                    make_error_msg(msg_to_send, ERROR_9,
                        atm, WRONG_SUM);
                    break;

                default: break;
            }
            msg_to_send->ID = index;
            break;
        }
        /* S-a primit putmoney */
        /* Se presupune ca e data corect de la terminal */
        case PUT_MONEY_T: {
            /* Adaug suma la soldul utilizatorului */
            DB->users[index].sold += atof(msg_recv.payload);

            msg_to_send->type = SUCCESS;
            sprintf(msg_to_send->payload,
                "%s Suma %.2f depusa cu succes",
                atm, atof(msg_recv.payload));
            msg_to_send->ID = index;
            break;
        }
        /* S-a primit quit */
        case QUIT_T: {
            DB->users[index].auth = 0;
            break;
        }
    }
    msg_to_send->msg_len = strlen(msg_to_send->payload) +
        3 * sizeof(int);
}

int check_login(char *args, database *DB, int *reg_no)
{
    char *token;
    int index;

    token = strtok(args, " ");

    /* Cautam numar_card in baza de date */
    for (index = 0; index < DB->number_of_users; index++)
        if (strcmp(token, DB->users[index].numar_card) == 0) {

            /* Verificam daca user-ul are deja o sesiune deschisa */
            if (DB->users[index].auth)
                return ERROR_2;

            /* Verificam daca e blocat cardul */
            if (DB->users[index].blocked) {
                *reg_no = index;
                return ERROR_5;
            }

            /* Verificam pinul */
            token = strtok(NULL, " ");
            if (strcmp(token, DB->users[index].pin) == 0) {

                /* Daca e corect il autentificam */
                DB->users[index].auth = 1;
                DB->users[index].tries = 0;
                *reg_no = index;
                return SUCCESS;
            } else {

                /* Daca e gresit mai are 2 incercari consecutive */
                DB->users[index].tries += 1;

                /* La 3 incercari esuate blocam cardul */
                if (DB->users[index].tries >= 3) {
                    DB->users[index].tries = 0;
                    DB->users[index].blocked = 1;
                    *reg_no = index;
                    return ERROR_5;
                }
                return ERROR_3;
            }
        }

    /* Daca nu am gasit cardul, card inexistent */
    /* Resetam numarul de pin-uri gresite pentru toti userii*/
    for (index = 0; index < DB->number_of_users; index++)
        DB->users[index].tries = 0;
    return ERROR_4;
}

/* Functie folosita de getmoney */
int check_sold(char *args, database *DB, int index)
{
    int sum = atoi(args);

    /* Suma ceruta nu e multiplu de 10 */
    if (sum % 10)
        return ERROR_9;

    /* Suma ceruta e mai mare decat soldul */
    if ((float) sum > DB->users[index].sold)
        return ERROR_8;

    /* Nicio eroare, scadem din sold suma ceruta */
    DB->users[index].sold -= (float) sum;
    return SUCCESS;
}

/* Proceseaza cererea UDP si pune in UDP_to_send raspunsul corespunzator */
void get_UDP_response(msg *UDP_to_send, msg UDP_recv, database *DB)
{
    const char unlock[] = "UNLOCK>";
    int ID = UDP_recv.ID;

    /* Cardul nu este in DB, eroare */
    if (ID < 0 || ID >= DB->number_of_users)
        make_error_msg(UDP_to_send, ERROR_4,
            unlock, UNREG_CARD);

    /* Daca s-a primit unlock */
    else if (UDP_recv.type == UNLOCK_T) {
        /* Si cardul nu este blocat, eroare */
        if (!DB->users[ID].blocked)
            make_error_msg(UDP_to_send, ERROR_6,
                unlock, TRANSACTION_FAILED);

        /* Altfel, cere parola secreta */
        else {
            UDP_to_send->type = GIVE_PASS;
            sprintf(UDP_to_send->payload,
                "%s Trimite parola secreta", unlock);
        }

    /* Daca s-a primit parola secreta o verificam cu cea din DB */
    } else if (UDP_recv.type == PASS) {
        if (strcmp(UDP_recv.payload,
            DB->users[ID].parola_secreta) == 0) {

            /* S-a dat parola corecta */
            DB->users[ID].blocked = 0;
            UDP_to_send->type = SUCCESS;
            sprintf(UDP_to_send->payload,
            "%s Client deblocat", unlock);
        /* Altfel, eroare */
        } else
            make_error_msg(UDP_to_send, ERROR_7,
                unlock, UNLOCK_FAILED);
    }

    UDP_to_send->ID = ID;
    UDP_to_send->msg_len = strlen(UDP_to_send->payload) +
            3 * sizeof(int);
}

/* Se creaza payload-ul unui mesaj de eroare */
void make_error_msg(msg *msg_to_send, int type,
    const char *service, char *err_msg)
{
    msg_to_send->type = type;
    sprintf(msg_to_send->payload, "%s %d : %s",
        service, type, err_msg);
}

void make_welcome_msg(msg *msg_to_send, char *nume, char *prenume, const char *atm)
{
    msg_to_send->type = LOGIN_T;
    sprintf(msg_to_send->payload, "%s Welcome %s %s", atm, nume, prenume);
}

void make_logout_msg(msg *msg_to_send, const char *atm)
{
    msg_to_send->type = LOGOUT_T;
    sprintf(msg_to_send->payload, "%s Deconectare de la bancomat", atm);
}

void make_get_money_msg(msg *msg_to_send, const char *atm, int sum)
{
    msg_to_send->type = SUCCESS;
    sprintf(msg_to_send->payload, "%s Suma %d retrasa cu succes", atm, sum);    
}
