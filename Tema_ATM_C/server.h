#ifndef _SERVER_H
#define _SERVER_H 1

typedef struct user_s
{
	char nume[13];
	char prenume[13];
	char numar_card[7];
	char pin[5];
	char parola_secreta[17];
	float sold;
	int tries;
	int auth;
	int blocked;

} user;

typedef struct database_s
{
	int number_of_users;
	user *users;
} database;

void init_database(database *DB, char *users_data_file);
void get_answer_for_msg(msg *msg_to_send, msg msg_recv, database *DB);
void get_UDP_response(msg *UDP_to_send, msg UDP_recv, database *DB);
int check_login(char *args, database *DB, int *reg_no);
int check_sold(char *args, database *DB, int index);
void make_error_msg(msg *msg_to_send, int type, const char *service, char *err_msg);
void make_welcome_msg(msg *msg_to_send, char *nume, char *prenume, const char *atm);
void make_logout_msg(msg *msg_to_send, const char *atm);
void make_get_money_msg(msg *msg_to_send, const char *atm, int sum);

#endif
