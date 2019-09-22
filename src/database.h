#ifndef DATABASE_H
#define DATABASE_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <mysql.h>



struct database_conn_param {
	char *username;          /* database user.*/
	char *password;          /* database user password.*/
	char *host;              /* database host.*/
	unsigned int port;       /* database listening port.*/
	char *database;          /* database schema name.*/
	char *sock;              /* database sock path.*/
	unsigned long clientflag;/* client option.*/
};

struct query_result_set {
	size_t no_row;    /* number of row returned.*/
	size_t no_fields; /* number of fields returned.*/
	char **result_set;/* array of string with the result.*/
};

/**
 * db_connect() - connect to an active istance of the database.
 * @param: database connection info.
 * 
 * db_connect() will establish an active connection to the database.
 *
 * Return: if success return a valid MYSQL istance, otherwise NULL is returned.
 */
MYSQL *db_connect(struct database_conn_param *param);

/**
 * db_close() - close an active connection to the database.
 * @mysql: active istance of a MYSQL returned by db_connect().
 */
void db_close(MYSQL *mysql);

/**
 * db_query() - send a query to the database and return a list of 
 *              result set.
 * @mysql: active connection to the database.
 * @query: the query to execute on the database.
 * @expected_rs: number of result set the query expect
 *               (example procedure return multiple result set).
 *
 * db_query() will send the @query to the database to an active
 * istance of @mysql database and return.
 * 
 * 
 * Return: in case of success db_query() will return an array long
 * @expected_rs of struct `query_result_set`, NULL in case of failure.
 */
struct query_result_set *db_query(MYSQL *mysql, const char *query, size_t expected_rs);

/**
 * db_statement() - execute a sql statement.
 * @mysql: active connection to the database.
 * @statement: the statement to execute on the database.
 *
 * db_statement() will send a statement to execute to the database.
 *
 * Return: if successfull return 0, otherwise return -1.
 */
int db_statement(MYSQL *mysql, const char *statement);

/**
 * db_quote_string() - create a valid sql string.
 * @mysql: active connection to the database.
 * @str: string to validate.
 * @quote_str_len: where to store the lenght of the new string.
 *
 * db_quote_string() will allocate on the heap a valid sql string from
 * @str and put the new lenght in @quote_str_len.
 * This is accomplished by mysql_real_escape_string().
 *
 * Return: in case of success return the new allocate string, NULL otherwise.
 */
char *db_quote_string(MYSQL *mysql, const char *str, size_t *quote_str_len);

/**
 * free_query_result_set() - free a query_result_set struct's returned by
 * db_query().
 * @rs: query_result_set to free.
 * @no_rs: number of structs to free, this should be the number of
 * result set returned by @db_query.
 */
void free_query_result_set(struct query_result_set *rs, size_t no_rs);

#endif
