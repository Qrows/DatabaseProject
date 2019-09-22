#include "database.h"

MYSQL *db_connect(struct database_conn_param *settings) {
	MYSQL *con = NULL;
	if (settings == NULL) {
		return NULL;
	}
	con = mysql_init(NULL);
	if (con == NULL) return NULL;
	if (mysql_real_connect(con, settings->host, settings->username,
			       settings->password, settings->database, settings->port,
			       settings->sock, settings->clientflag) == NULL) {
		fprintf(stderr, "%s\n", mysql_error(con));
		return NULL;
	}
	return con;
}

void db_close(MYSQL *mysql) {
	if (mysql)
		mysql_close(mysql);
}

char *db_quote_string(MYSQL *mysql, const char *str, size_t *quote_str_len) {
	char *qstr = NULL;
	size_t str_len = 0;
	size_t qstr_len = 0;
	unsigned long status = 0;
	if (str == NULL) {
		return NULL;
	}
	str_len = strlen(str);
	/* By doc, escaped string lenght is at max old string len times 2 plus 1.*/
	qstr_len = 2 * str_len + 1;
	qstr = malloc(qstr_len);
	if (qstr == NULL) {
		return NULL;
	}
	memset(qstr, 0, qstr_len);
	status = mysql_real_escape_string(mysql, qstr, str, str_len);
	if (status == 0) {
		fprintf(stderr, "%s\n", mysql_error(mysql));
		free(qstr);
		return NULL;
	}
	if (quote_str_len)
		*quote_str_len = qstr_len;
	return qstr;
}

static MYSQL_RES *store_query(struct query_result_set *result, MYSQL *mysql) {
	/* send query to db and store the result */
	MYSQL_RES *res = NULL;
	res = mysql_store_result(mysql);
	if (res == NULL && mysql_errno(mysql) != 0) {
		fprintf(stderr, "%s:%s\n", "mysql_store_result()", mysql_error(mysql));
		return NULL;
        } else if (res == NULL) {
		return NULL;
        }
        result->no_fields = mysql_field_count(mysql);
	result->no_row = mysql_num_rows(res);
	result->result_set =
		malloc(result->no_row * result->no_fields * sizeof(*result));
	if (result == NULL) {
		mysql_free_result(res);
		return NULL;
	}
	return res;
}

static int save_fields(struct query_result_set *result, MYSQL_ROW *row,
                       unsigned long long row_index, unsigned long *lengths) {
	/* Copy the content of a MYSQL_ROW inside result->result_set[row_index *
	 * result->no_fields]. */
	for (unsigned long long i = 0; i < result->no_fields; ++i) {

		result->result_set[row_index * result->no_fields + i] =
			strndup(*(*row + i), lengths[i]);
		if (result->result_set[row_index * result->no_fields + i] == NULL) {
			for (unsigned long long j = 0;
			     j < row_index * result->no_fields + row_index; ++j) {
				free(result->result_set[j]);
				return 1;
			}
		}
	}
	return 0;
}

static int process_result_set(MYSQL *mysql, MYSQL_RES *res,
                              struct query_result_set *result) {
	/* create a struct query_result_set from a MYSQL_RES*/
	MYSQL_ROW row;
	unsigned long *lengths;
	int status = 0;
	for (unsigned long long i = 0; i < result->no_row; ++i) {
		row = mysql_fetch_row(res);
		if (row == NULL) {
			fprintf(stderr, "%s:%s\n", "mysql_fetch_row()", mysql_error(mysql));
			return 1;
		}
		lengths = mysql_fetch_lengths(res);
		if (lengths == NULL) {
			return 1;
		}
		status = save_fields(result, &row, i, lengths);
		if (status) {
			return 1;
		}
	}
	return 0;
}

static int send_query(MYSQL *mysql, const char *query) {
	int status = 0;
	status = mysql_real_query(mysql, query, strlen(query));
	if (status) {
		fprintf(stderr, "%s:%s\n", "mysql_real_query()", mysql_error(mysql));
		return -1;
	}
	return 0;
}

int db_statement(MYSQL *mysql, const char *statement) {
	if (mysql == NULL || statement == NULL) {
		return -1;
	}
	return send_query(mysql, statement);
}


struct query_result_set *db_query(MYSQL *mysql, const char *query, size_t expected_rs) {
	int status = 0;
	MYSQL_RES *res = NULL;
	struct query_result_set *result = NULL;
	status = send_query(mysql, query);
	if (status) {
		return NULL;
	}
	result = malloc(sizeof(*result) * expected_rs);
        if (result == NULL) {
		return NULL;
        }
	memset(result, 0, sizeof(*result));
        for (size_t i = 0; i < expected_rs; ++i) {
		res = store_query(result + i, mysql);
		if (res == NULL && mysql_errno(mysql) != 0) {
			/* error */
			return NULL;
		} else if (res == NULL) {
			/* no result */
			memset(&result[i], 0, sizeof(result[i]));
                } else {
			status = process_result_set(mysql, res, result + i);
			if (status) {
				free(result);
				mysql_free_result(res);
				return NULL;
			}
		}
		mysql_free_result(res);
		/* set result */
		
		status = mysql_next_result(mysql);
		if (status > 0) {
			return NULL;
		}
        }
        return result;
}

void free_query_result_set(struct query_result_set *rs, size_t no_rs) {
	for (size_t z = 0; z < no_rs; ++z) {
          if (rs[z].result_set) {
		  for (size_t i = 0; i < rs->no_row; ++i) {
			  for (size_t j = 0; j < rs[z].no_fields; ++j) {
				  free(rs[z].result_set[i * rs[z].no_fields + j]);
			  }
		  }
		  free(rs[z].result_set);
	  }
	}
	free(rs);
}
