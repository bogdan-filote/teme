# Makefile

# Compilator
CC = gcc
CCFLAGS = -Wall -g

# Biblioteci
LIBSOCKET = -lnsl

# Denumiri executabile
SRV = server
CLT = client

# Portul pe care asculta serverul
PORT = 7777

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

# Numele fisierului cu informatii despre useri
FILE_NAME = users_data_file

build: $(SRV) $(CLT)

$(SRV): $(SRV).o server_side.o
	$(CC) -o $(SRV) $(LIBSOCKET) $(SRV).o server_side.o

$(CLT): $(CLT).o client_side.o
	$(CC) -o $(CLT) $(LIBSOCKET) $(CLT).o client_side.o

$(SRV).o: $(SRV).c $(SRV).h common.h

$(CLT).o: $(CLT).c $(CLT).h common.h

server_side.o: server_side.c $(SRV).h common.h

client_side.o: client_side.c $(CLT).h common.h

.PHONY: clean runserver runclient

# Ruleaza serverul
runserver:
	./$(SRV) ${PORT} ${FILE_NAME}

# Ruleaza clientul 	
runclient:
	./$(CLT) ${IP_SERVER} ${PORT}

clean:
	rm -f *.log *.o *~
	rm -f $(SRV) $(CLT)


