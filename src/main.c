#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "business.h"

static const char *HELP_STRING =
	"Uso: > COMANDO [OPZIONE]\n"
	"questo programma è una directory aziendale con lo scopo di gestire\n"
	"i dipendenti di un azienda, le mansioni svolte da quest'ultimi\n"
	"e gli spazi che l'azienda ha a disposizione.\n\n"
	"Comandi presenti:\n"
	"  init\n"
	"    - `init`: genera file di configurazione.\n"
	"  ricerca\n"
	"    - `ricerca dipendente`: trova i recapiti di un dipendente.\n"
	"    - `ricerca ufficio`: trova le informazioni riguardanti un ufficio\n"
	"       che contiene la postazione associata a tale numero di telefono.\n"
	"  lista\n"
	"    - `lista dipendenti`: stampa una lista dei dipendenti da trasferire.\n"
	"    - `lista uffici`: stampa una lista di tutti gli uffici dell'azienda.\n"
	"    - `lista postazioni`: dato un dipendente ritorna tutti gli uffici in cui può essere trasferito.\n"
	"  modifica\n"
	"    - `modifica mansione`: modifica la mansione di un dipendente.\n"
	"  trasferisci\n"
	"    - `trasferisci postazione`: trasferisci un dipendente in una nuova postazione libera.\n"
	"    - `trasferisci swap`: scambia la postazione di due dipendenti da trasferire.\n"
	"  help\n"
	"    - stampa questo messaggio.\n"
	"  exit\n"
	"    - esci dal programma.\n";

int generate_cfg(void)
{
	FILE *cfg = NULL;
	const char *cfg_format = 
		"# CONFIG\n"
		"# DATABASE OPTION\n"
		"HOST %s\n"
		"PORT %s\n"
		"SOCKET %s\n"
		"# DEFAULT LOGIN\n"
		"# USERNAME \n"
		"# PASSWORD \n";
	const char *host = "localhost";
	const char *port = "3306";
	const char *socket = "/var/run/mysqld/mysqld.sock";
	int status = 0;
	cfg = fopen("config", "w+");
	if (cfg == NULL)
		return 1;
	status = fprintf(cfg, cfg_format, host, port, socket);
	if (status < 0) {
		fclose(cfg);
		exit(EXIT_FAILURE);
	}
	fclose(cfg);
	return EXIT_SUCCESS;
}

static int split_line(char *cfgline, size_t size, char **name, char **value)
{
	char *ptr = NULL;
	*name = cfgline;
	for (ptr = cfgline; *ptr != '\0' && *ptr != ' '; ++ptr)
		;
	*ptr = '\0';
	// trim whitespace
	for (++ptr; *ptr != '\0' && (*ptr == ' ' || *ptr == '\t'); ++ptr)
		;
	*value = ptr;
	// trim ending whitespace
	for (; *ptr != '\0' && *ptr != ' ' && *ptr != '\n'; ++ptr)
		;
	*ptr = '\0';
	return 0;
}

static void parse_value(struct database_conn_param *param, char *pname,
			char *value)
{
	if (strcmp(pname, "HOST") == 0) {
		param->host = strdup(value);
		if (param->host == NULL)
			exit(EXIT_FAILURE);
	} else if (strcmp(pname, "PORT") == 0) {
		param->port = atoi(value);
	} else if (strcmp(pname, "SOCKET") == 0) {
		param->sock = strdup(value);
		if (param->sock == NULL)
			exit(EXIT_FAILURE);
	} else if (strcmp(pname, "USERNAME") == 0) {
		param->username = strdup(value);
		if (param->username == NULL)
			exit(EXIT_FAILURE);
	} else if (strcmp(pname, "PASSWORD") == 0) {
		param->password = strdup(value);
		if (param->password == NULL)
			exit(EXIT_FAILURE);
	} else {
	}
}

int read_cfg(struct database_conn_param *param, FILE *cfg)
{
	char *line = NULL;
	size_t line_len = 0;
	ssize_t rd = 0;
	while ((rd = getline(&line, &line_len, cfg)) > 0) {
		char *pname = NULL;
		char *value = NULL;
		if (*line != '#') {
			split_line(line, line_len, &pname, &value);
			parse_value(param, pname, value);
		}
	}
	free(line);
	return 0;
}

static void free_cfg(struct database_conn_param *param)
{
	if (param) {
		if (param->host)
			free(param->host);
		if (param->sock)
			free(param->host);
		if (param->username)
			free(param->host);
		if (param->password)
			free(param->host);
	}
}

int get_input(char *input_name, char *input, size_t max_input, int echo)
{
	struct termios newt, oldt;
	size_t i = 0;
	char c = 0;
	if (input_name == NULL || input == NULL)
		return 0;
	if (echo) {
		tcgetattr(STDIN_FILENO, &oldt);
		newt = oldt;
		newt.c_lflag &= ~(ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	}
	printf("%s ", input_name);
	while ((c = getchar()) != '\n' && c != EOF && i < max_input - 1) {
		input[i] = c;
		++i;
	}
	if (echo) {
		tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	}
	if (c == EOF) {
		putchar('\n');
		return 1;
	}
	input[i] = '\0';
	return 0;
}

static void free_worker_contacts(struct Worker_Contacts *wc)
{
	if (wc) {
		free(wc->fiscal_code);
		free(wc->name);
		free(wc->surname);
		free(wc->email);
		free(wc->office_email);
		free(wc->office_phone);
		free(wc->office);
		free(wc->building);
	}
}

static int do_search_worker(struct database_conn_param *param)
{
	char name[65] = { 0 };
	char surname[65] = { 0 };
	struct Worker_Contacts *wr = NULL;
	size_t no_worker = 0;
	int res = 0;
	printf("%s\n", "Ricerca Dipendente");
	res = get_input("Nome:", name, 64, 0);
	if (res)
		return EXIT_FAILURE;
	res = get_input("Cognome:", surname, 64, 0);
	if (res)
		return EXIT_FAILURE;
	res = search_worker(param, (*name == '\0') ? NULL : name,
			    (*surname == '\0') ? NULL : surname, &wr,
			    &no_worker);
	if (res) {
		fprintf(stderr, "%s: %s\n", "Errore", get_error(res));
		return EXIT_FAILURE;
	}
	putchar('\n');
	for (size_t i = 0; i < no_worker; ++i) {
		printf("Codice Fiscale: %s\n"
		       "Nome: %s\n"
		       "Cognome: %s\n"
		       "Email: %s\n"
		       "Email Ufficio: %s\n"
		       "Telefono Ufficio: %s\n"
		       "Ufficio: %s\n"
		       "Sede: %s\n\n",
		       wr[i].fiscal_code, wr[i].name, wr[i].surname,
		       wr[i].email, wr[i].office_email, wr[i].office_phone,
		       wr[i].office, wr[i].building);
	}
	for (size_t i = 0; i < no_worker; ++i)
		free_worker_contacts(&wr[i]);
	free(wr);
	return EXIT_SUCCESS;
}

static void free_worker(struct Worker *wk)
{
	if (wk) {
		free(wk->fiscal_code);
		free(wk->name);
		free(wk->surname);
		free(wk->task);
	}
}

static void free_office(struct Office *of)
{
	if (of) {
		free(of->name);
		free(of->floor);
		free(of->building);
		if (of->task) {
			free(of->task);
		}
		if (of->worker) {
			for (size_t i = 0; i < of->no_worker; ++i)
				free_worker(&of->worker[i]);
			free(of->worker);
		}
	}
}

static int do_search_office(struct database_conn_param *param)
{
	char phone[33] = { 0 };
	int res = 0;
	struct Office *office = NULL;
	int status = 0;
	res = get_input("Telefono:", phone, 32, 0);
	if (res)
		return EXIT_FAILURE;
	status = search_office(param, phone, &office);
	if (status) {
		fprintf(stderr, "%s: %s\n", "Errore", get_error(status));
		return EXIT_FAILURE;
	}
	putchar('\n');
	printf("%s: %s\n", "Ufficio", office->name);
	printf("%s: %s\n", "Piano", office->floor);
	printf("%s: %s\n", "Edificio", office->building);
	printf("%s: %lu\n", "No. Dipendenti", office->no_worker);
	putchar('\n');
	if (office->worker) {
		for (size_t i = 0; i < office->no_worker; ++i) {
			printf("%s: %s\n", "Codice Fiscale",
			       office->worker[i].fiscal_code);
			printf("%s: %s\n", "Nome", office->worker[i].name);
			printf("%s: %s\n", "Cognome",
			       office->worker[i].surname);
			printf("%s: %s\n", "Da Trasferire",
			       (office->worker[i].transfer) ? "[V]" : "[X]");
			putchar('\n');
		}
	}
	free_office(office);
	return EXIT_SUCCESS;
}

static int do_list_transfer_worker(struct database_conn_param *param)
{
	struct Worker *worker = NULL;
	size_t no_worker = 0;
	int status = 0;
	status = list_to_transfer_worker(param, &worker, &no_worker);
	if (status) {
		fprintf(stderr, "%s: %s\n", "Errore", get_error(status));
		return EXIT_FAILURE;
	}
	putchar('\n');
	for (size_t i = 0; i < no_worker; ++i) {
		printf("%s: %s\n", "Codice Fiscale", worker[i].fiscal_code);
		printf("%s: %s\n", "Nome", worker[i].name);
		printf("%s: %s\n", "Cognome", worker[i].surname);
		printf("%s: %s\n", "Mansione", worker[i].task);
		putchar('\n');
	}
	for (size_t i = 0; i < no_worker; ++i)
		free_worker(&worker[i]);
	free(worker);
	return EXIT_SUCCESS;
}

static int do_list_office(struct database_conn_param *param)
{
	struct Office *office = NULL;
	size_t no_office = 0;
	int status = 0;
	status = list_office(param, &office, &no_office);
	if (status) {
		fprintf(stderr, "%s: %s\n", "Errore", get_error(status));
		return EXIT_FAILURE;
	}
	putchar('\n');
	for (size_t i = 0; i < no_office; ++i) {
		printf("%s: %s\n", "Ufficio", office[i].name);
		printf("%s: %s\n", "Piano", office[i].floor);
		printf("%s: %s\n", "Edificio", office[i].building);
		printf("%s: %s\n", "Mansione", office[i].task);
		putchar('\n');
	}
	free_office(office);
	free(office);
	return EXIT_SUCCESS;
}

static int do_transfer_list_position(struct database_conn_param *param)
{
	char worker[17] = { 0 };
	struct FreePosition *positions = NULL;
	size_t no_position = 0;
	int status = 0;
	status = get_input("Dipendente:", worker, 16, 0);
	if (status)
		return EXIT_FAILURE;
	status =
		list_transfer_position(param, worker, &positions, &no_position);
	if (status) {
		fprintf(stderr, "%s: %s\n", "Errore", get_error(status));
		return EXIT_FAILURE;
	}
	for (size_t i = 0; i < no_position; ++i) {
		printf("%s: %s\n", "Telefono", positions[i].phone);
		printf("%s: %s\n", "Ufficio", positions[i].office);
		printf("%s: %s\n", "Edificio", positions[i].building);
		printf("%s: %s\n", "Dipendente", positions[i].worker);
		putchar('\n');
	}
	for (size_t i = 0; i < no_position; ++i) {
		free(positions[i].phone);
		free(positions[i].office);
		free(positions[i].building);
		free(positions[i].worker);
	}
	free(positions);
	return EXIT_SUCCESS;
}

static int do_transfer_to_position(struct database_conn_param *param)
{
	char dip[17] = { 0 };
	char pos[33] = { 0 };
	int res = 0;
	res = get_input("Codice Fiscale Dipendente:", dip, 16, 0);
	if (res)
		return EXIT_FAILURE;
	res = get_input("Telefono Posizione:", pos, 32, 0);
	if (res)
		return EXIT_FAILURE;
	res = transfer_worker_to_free_position(param, dip, pos);
	if (res) {
		printf("%s\n", "Falimento!");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static int do_transfer_swap(struct database_conn_param *param)
{
	char dip1[17] = { 0 };
	char dip2[17] = { 0 };
	int res = 0;
	res = get_input("Codice Fiscale Dipendente:", dip1, 16, 0);
	if (res)
		return EXIT_FAILURE;
	res = get_input("Codice Fiscale Secondo Dipendente:", dip2, 16, 0);
	if (res)
		return EXIT_FAILURE;
	res = transfer_worker_swap(param, dip1, dip2);
	if (res) {
		printf("%s\n", "Falimento!");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

static int do_update_task(struct database_conn_param *param)
{
	char dip[17] = { 0 };
	char task[65] = { 0 };
	int res = 0;
	res = get_input("Codice Fiscale Dipendente:", dip, 16, 0);
	if (res)
		return EXIT_FAILURE;
	res = get_input("Mansione:", task, 64, 0);
	if (res)
		return EXIT_FAILURE;
	res = update_worker_task(param, dip, task);
	if (res) {
		printf("%s\n", "Fallimento!");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	int res = 0;
	char username[256] = { 0 };
	char pwd[256] = { 0 };
	FILE *cfg = 0;
	char command[512] = {0};
	struct database_conn_param settings = { 0 };
	/* 	{ */
	/* 	"qrowsxi", "qrowsxi", "localhost", */
	/* 	3306,      "Azienda", "/var/run/mysqld/mysqld.sock", */
	/* 	0 */
	/* }; */
	cfg = fopen("config", "r");
	if (cfg == NULL) {
		fprintf(stderr, "generating config file\n");
		generate_cfg();
		cfg = fopen("config", "r");
		if (cfg == NULL) {
			return EXIT_FAILURE;
		}
	}
	read_cfg(&settings, cfg);
	fclose(cfg);
	settings.database = "Azienda";
	if (settings.host == NULL || settings.database == NULL ||
	    settings.sock == NULL) {
		exit(EXIT_FAILURE);
	}
	if (settings.username == NULL || settings.password == NULL)
		printf("%s:\n", "login prompt");
	if (settings.username == NULL) {
		settings.username = malloc(256);
		if (settings.username == NULL) {
			return EXIT_FAILURE;
		}
		memset(settings.username, 0, 256);
		res = get_input("username:", settings.username, 255, 0);
		if (res) {
			return EXIT_FAILURE;
		}
	}
	if (settings.password == NULL) {
		settings.password = malloc(256);
		if (settings.password == NULL) {
			return EXIT_FAILURE;
		}
		memset(settings.password, 0, 256);
		res = get_input("password:", settings.password, 255, 1);
		if (res) {
			return EXIT_FAILURE;
		}
	}
	putchar('\n');
	printf("%s\n", HELP_STRING);	
	while (!feof(stdin)) {
		char *cmd = NULL;
		char *opt = NULL;
		res = get_input("> ", command, 511, 0);
		if (res) {
			return EXIT_FAILURE;
		}
		if (strcmp(command, "init") == 0) {
			fprintf(stderr, "generating config file\n");
			generate_cfg();
			return EXIT_SUCCESS;
		}		
		split_line(command, 512, &cmd, &opt);
		if (strcmp("ricerca", cmd) == 0) {
			if (strcmp("dipendente", opt) == 0) {
				res = do_search_worker(&settings);
			} else if (strcmp("ufficio", opt) == 0) {
				res = do_search_office(&settings);
			} else {
				fprintf(stderr, "%s\n", "Comando non riconosciuto!");
				return EXIT_FAILURE;
			}
		} else if (strcmp("lista", cmd) == 0) {
			if (strcmp("dipendenti", opt) == 0) {
				res = do_list_transfer_worker(&settings);
				if (res) {
					fprintf(stderr, "%s\n", HELP_STRING);					
					return EXIT_FAILURE;
				}				
			} else if (strcmp("uffici", opt) == 0) {
				res = do_list_office(&settings);
				if (res) {
					fprintf(stderr, "%s\n", HELP_STRING);					
					return EXIT_FAILURE;
				}				
			} else if (strcmp("postazioni", opt) == 0) {
				res = do_transfer_list_position(&settings);
				if (res) {
					fprintf(stderr, "%s\n", HELP_STRING);					
					return EXIT_FAILURE;
				}				
			} else {
				fprintf(stderr, "%s\n", HELP_STRING);
				return EXIT_FAILURE;
			}
		} else if (strcmp("modifica", cmd) == 0) {
			if (strcmp("mansione", opt) == 0) {
				res = do_update_task(&settings);
				if (res) {
					fprintf(stderr, "%s\n", HELP_STRING);					
					return EXIT_FAILURE;
				}
			} else {
				fprintf(stderr, "%s\n", HELP_STRING);
				return EXIT_FAILURE;
			}
		} else if (strcmp("trasferisci", cmd) == 0) {
			if (strcmp("postazione", opt) == 0) {
				res = do_transfer_to_position(&settings);
				if (res) {
					fprintf(stderr, "%s\n", HELP_STRING);					
					return EXIT_FAILURE;
				}				
			} else if (strcmp("swap", opt) == 0) {
				res = do_transfer_swap(&settings);
				if (res) {
					fprintf(stderr, "%s\n", HELP_STRING);					
					return EXIT_FAILURE;
				}				
			} else {
				fprintf(stderr, "%s\n", HELP_STRING);
				return EXIT_FAILURE;
			}
		} else if (strcmp("help", cmd) == 0) {
			fprintf(stderr, "%s\n", HELP_STRING);
		} else if (strcmp("exit", cmd) == 0) {
			return EXIT_SUCCESS;
		} else if (strcmp("", cmd) == 0) {
			memset(command, 0, sizeof(command));
			continue;
		} else {
			fprintf(stderr, "%s\n", HELP_STRING);
			return EXIT_FAILURE;
		}
		memset(command, 0, sizeof(command));
	}
	return EXIT_SUCCESS;
}
