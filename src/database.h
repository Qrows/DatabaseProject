#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <mysql/mysql.h>



struct database_conn_param {
	char *username;
	char *password;
	char *host;
	unsigned int port;
	char *database;
};

struct query_result_set {
	size_t no_row;
	size_t no_fields;
	char **result_set;
};

struct query_result_set *query(MYSQL *mysql, const char *query);
