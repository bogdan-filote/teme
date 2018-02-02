#ifndef _CLIENT_H
#define _CLIENT_H 1

void get_msg_for_command(msg *msg_to_send, char *buffer);
int client_command_check(msg *msg_to_send, int auth);
void make_error_msg(msg *msg_to_send, int errno, char *err_msg);
void make_msg(msg *msg_to_send, int type);
void make_msg_args(msg *msg_to_send, int type, const char *delim);
void make_UDP_msg(msg *UDP_to_send, int type, int ID, char *buffer);

#endif
