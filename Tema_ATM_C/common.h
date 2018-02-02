#ifndef _COMMON_H
#define _COMMON_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFLEN 256
#define LINE_LENGTH	128
#define INPUT_ERROR	-10
#define IN_ERR_MSG	"Comanda gresita"
#define ERROR_9	-9
#define WRONG_SUM	"Suma nu e multiplu de 10"
#define ERROR_8	-8
#define INSUF_FUNDS	"Fonduri insuficente"
#define ERROR_7	-7
#define UNLOCK_FAILED	"Deblocare esuata"
#define ERROR_6	-6
#define TRANSACTION_FAILED	"Tranzactie esuata"
#define ERROR_5	-5
#define BLOCKED_CARD	"Card blocat"
#define ERROR_4	-4
#define UNREG_CARD	"Numar card inexistent"
#define ERROR_3	-3
#define WRONG_PIN	"Pin gresit"
#define ERROR_2	-2
#define OPENED_SESSION	"Sesiune deja deschisa"
#define ERROR_1	-1
#define UNAUTH	"Clientul nu este autentificat"
#define SUCCESS	0
#define LOGIN_T	1
#define LOGIN 	"login"
#define	LOGOUT_T	2
#define LOGOUT 	"logout"
#define	LIST_SOLD_T	3
#define LIST_SOLD 	"listsold"
#define GET_MONEY_T	4
#define GET_MONEY 	"getmoney"
#define PUT_MONEY_T	5
#define PUT_MONEY 	"putmoney"
#define UNLOCK_T	6
#define UNLOCK 	"unlock"
#define QUIT_T	7
#define QUIT 	"quit"
#define GIVE_PASS	8
#define PASS 	9

#define DIE(assertion, call_description)				\
	do {								\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",			\
					__FILE__, __LINE__);		\
			perror(call_description);			\
			exit(EXIT_FAILURE);				\
		}							\
	} while(0)

typedef struct msg_s
{
	int type;
	int ID;
	char payload[LINE_LENGTH];
	int msg_len;
} msg;

#endif
