#include "database.h"

struct query_result_set *query(MYSQL *mysql, const char *query)
{
	int status = 0;
	unsigned long long no_fields = 0;
	MYSQL_RES *res = NULL;
	MYSQL_ROW row;
	my_ulonglong no_row = 0;
	unsigned long *lengths = NULL;
	struct query_result_set *result = NULL;
	result = malloc(sizeof(*result));
        if (result == NULL) {
		return NULL;
        }
        status = mysql_real_query(mysql, query, strlen(query));
        if (status) {
		fprintf(stderr, "%s:%s\n","mysql_real_query()" ,mysql_error(mysql));
		return NULL;
        }

	res = mysql_store_result(mysql);
        if (res == NULL) {
		fprintf(stderr, "%s:%s\n","mysql_store_result()" ,mysql_error(mysql));
		return NULL;
        }
	result->no_fields = mysql_field_count(mysql);
	result->no_row = mysql_num_rows(res);
	fprintf(stderr, "no_fields: %lu\nno_row: %lu\n",
		result->no_fields,
		result->no_row);
	result->result_set = malloc(result->no_row * result->no_fields * sizeof(*result));
        if (result == NULL) {
		mysql_free_result(res);
		return NULL;
        }
	
	for (unsigned long long i = 0; i < result->no_row; ++i) {
		row = mysql_fetch_row(res);
                if (row == NULL) {
			fprintf(stderr, "%s:%s\n","mysql_fetch_row()" ,mysql_error(mysql));			   
			free(result);
			mysql_free_result(res);
			return NULL;
                }
                lengths = mysql_fetch_lengths(res);
                if (lengths == NULL) {
			fprintf(stderr, "%s:%s\n", "mysql_fetch_lenghts()", mysql_error(mysql));			
			free(result);
			mysql_free_result(res);
			return NULL;
                }
                for (unsigned long long j = 0; j < result->no_fields; ++j) {
			result->result_set[i*result->no_fields + j] = malloc(lengths[j] + 1);
                        if (result->result_set[i + j] == NULL) {
				mysql_free_result(res);
                                for (unsigned long long z = 0; z < i*result->no_fields + j; ++z) {
					free(result->result_set[z]);
                                }
				free(result);
				return NULL;
                        }
			strncpy(result->result_set[i*result->no_fields + j], row[j], lengths[j]);
			result->result_set[i*result->no_fields + j][lengths[j]] = '\0';
                }
	}
        mysql_free_result(res);
        return result;
}
