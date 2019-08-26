#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "database.h"

static const char *HELP_STRING =
    "Uso: diraz COMANDO [OPZIONI]\n"
    "diraz è una directory aziendale con lo scopo di gestire"
    "i dipendenti di un azienda, le mansioni svolte da quest'ultimi"
    "e gli spazi che l'azienda ha a disposizione.\n\n"
    "Comandi presenti:\n"
    "  search\n"
    "    - Con opzione -w (worker) trova i recapiti di un dipendente.\n"
    "    - Con opzione -p (phone) trova le informazioni riguardanti "
    "Postazione, ufficio"
    " che contiene la postazione associata a tale numero di telefono.\n"
    "  list\n"
    "    - con opzione -w o mostra la storia lavorativa di un "
    "dipendente.\n"
    "    - con opzione -t o --transferimento mostra una lista di tutti i"
    " dipendenti da trasferire.\n"
    "  transfer\n"
    "    Comando eseguibile solamente da un utente del settore degli spazi.\n"
    "  update\n";

/* int main(int argc, char *argv[]) */
/* { */
/* 	int opt = 0; */
/*         if (argc < 2) { */
/* 		fprintf(stderr, "%s\n", HELP_STRING); */
/* 		exit(EXIT_FAILURE); */
/*         } */
/*         if (strcmp(argv[1], "search") == 0) { */
/* 		/\* caso insert*\/ */
/* 		/\* TODO: getopt prendere argomento da cmd (nome, cognome)*\/ */
/*                 while ((opt = getopt(argc - 1, argv + 1, ":wp")) != -1) { */
/*                   switch (opt) { */
/* 		  case 'w': */
/* 			  break; */
/* 		  case 'p': */
/* 			  break; */
/* 		  default: /\* '?' *\/ */
/* 			  fprintf(stderr, "%s\n", HELP_STRING); */
/* 			  exit(EXIT_FAILURE); */
/* 		  } */
/*                 } */
/*         } else if (strcmp(argv[1], "list") == 0) { */
/* 		/\* caso list*\/ */
/* 	} else if (strcmp(argv[1], "transfer") == 0) { */
/* 		/\* caso transfer*\/ */
/* 	} else if (strcmp(argv[1], "update") == 0) { */
/* 		/\* caso update *\/ */
/* 	} else { */
/* 		// fprintf(stderr, "%s\n", HELP_STRING); */
/* 		return EXIT_FAILURE; */
/* 	} */
/* 	return 0; */
/* } */

int main(int argc, char *argv[])
{
	MYSQL *conn = NULL;
	int res = 0;
        struct database_conn_param settings = {"qrowsxi",
					       "qrowsxi",
					       "localhost",
					       3306,
					       "Azienda"};
	struct query_result_set *rs = NULL;
        if (argc != 2) {
		fprintf(stderr, "program query\n");
		return 1;
        }
	
        conn = mysql_init(NULL);
        if (mysql_real_connect(conn, settings.host, settings.username,
                               settings.password, settings.database,
                               settings.port, "/var/run/mysqld/mysqld.sock", 0) == NULL) {
		fprintf(stderr, "%s\n", mysql_error(conn));
		return 1;
        }
	rs = query(conn, argv[1]);
        if (rs) {
		fprintf(stderr, "%s\n", "rs != NULL");
		for (size_t i = 0; i < rs->no_row; ++i) {
			putchar('|');
			for (size_t j = 0; j < rs->no_fields; ++j) {
				printf(" %s | ", rs->result_set[i*rs->no_fields + j]);
			}
			putchar('\n');
		}
        }
        mysql_close(conn);
        return 0;
}
