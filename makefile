CC=gcc
CFLAGS=`mariadb_config --cflags`
LIBS=`mariadb_config --libs` 
SRC_FOLDER=./src
MAIN=applicazione
CFILE=$(SRC_FOLDER)/database.c $(SRC_FOLDER)/business.c $(SRC_FOLDER)/main.c
DEBUG=-g


$(MAIN): $(CFILE)
	$(CC) $(DEBUG) -o $(MAIN) $(CFLAGS) $(CFILE) $(LIBS)
