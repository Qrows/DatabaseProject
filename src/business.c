#include "business.h"

char *get_error(int errnum)
{
	return errmsg[errnum % sizeof(errmsg)];
}

static int init_worker_contacts(struct Worker_Contacts *wk, char *fc,
				char *name, char *surname, char *email,
				char *office_phone, char *office_email,
				char *office, char *building)
{
	/* 
	 * init a worker struct copy all the input parameters using strdup()
	 */
	wk->fiscal_code = strdup(fc);
	if (wk->fiscal_code == NULL) {
		return 1;
	}
	wk->name = strdup(name);
	if (wk->name == NULL) {
		free(wk->fiscal_code);
		return 1;
	}
	wk->surname = strdup(surname);
	if (wk->surname == NULL) {
		free(wk->fiscal_code);
		free(wk->name);
		return 1;
	}
	wk->email = strdup(email);
	if (wk->email == NULL) {
		free(wk->fiscal_code);
		free(wk->name);
		free(wk->surname);
		return 1;
	}
	wk->office_email = strdup(office_email);
	if (wk->office_email == NULL) {
		free(wk->fiscal_code);
		free(wk->name);
		free(wk->surname);
		free(wk->email);
		return 1;
	}
	wk->office_phone = strdup(office_phone);
	if (wk->office_phone == NULL) {
		free(wk->fiscal_code);
		free(wk->name);
		free(wk->surname);
		free(wk->email);
		free(wk->office_email);
		return 1;
	}
	wk->office = strdup(office);
	if (wk->office == NULL) {
		free(wk->fiscal_code);
		free(wk->name);
		free(wk->surname);
		free(wk->email);
		free(wk->office_email);
		free(wk->office_phone);
		return 1;
	}
	wk->building = strdup(building);
	if (wk->building == NULL) {
		free(wk->fiscal_code);
		free(wk->name);
		free(wk->surname);
		free(wk->email);
		free(wk->office_email);
		free(wk->office_phone);
		free(wk->office);
		return 1;
	}
	return 0;
}

static void free_worker_contact(struct Worker_Contacts *wk)
{
	if (wk) {
		free(wk->fiscal_code);
		free(wk->name);
		free(wk->surname);
		free(wk->email);
		free(wk->office_email);
		free(wk->office_phone);
		free(wk->office);
		free(wk->building);
		free(wk);
	}
}

int search_worker(struct database_conn_param *param, char *name, char *surname,
		  struct Worker_Contacts **result, size_t *no_worker)
{
	char *q_name = NULL;
	size_t q_name_len = 0;
	char *q_surname = NULL;
	size_t q_surname_len = 0;
	char *query_template = NULL;
	char *query = NULL;
	size_t query_len = 0;
	MYSQL *mysql = NULL;
	struct query_result_set *rs = NULL;
	struct Worker_Contacts *res = NULL;
	// check input
	if (param == NULL || (name == NULL && surname == NULL) ||
	    result == NULL || no_worker == NULL) {
		return INVALID_PARAMETER;
	}
	// call and set up query to do.
	mysql = db_connect(param);
	if (mysql == NULL)
		return LOGIN_FAILED;
	if (name == NULL) {
		// prepare query, quote input against sql injections.
		query_template = "CALL RicercaDipendente(NULL, '%s');";
		q_surname = db_quote_string(mysql, surname, &q_surname_len);
		if (q_surname == NULL) {
			db_close(mysql);
			return OUT_OF_MEMORY;
		}
		query_len = strlen(query_template) + q_surname_len + 1;
		query = malloc(query_len);
		if (query == NULL) {
			db_close(mysql);
			free(q_surname);
			return OUT_OF_MEMORY;
		}
		memset(query, 0, query_len);
		snprintf(query, query_len, query_template, q_surname);
		free(q_surname);
	} else if (surname == NULL) {
		// prepare query, quote input against sql injections.		
		query_template = "CALL RicercaDipendente('%s', NULL);";
		q_name = db_quote_string(mysql, name, &q_name_len);
		if (q_name == NULL) {
			db_close(mysql);
			return OUT_OF_MEMORY;
		}
		query_len = strlen(query_template) + q_name_len + 1;
		query = malloc(query_len);
		if (query == NULL) {
			free(q_name);
			db_close(mysql);
			return OUT_OF_MEMORY;
		}
		memset(query, 0, query_len);
		snprintf(query, query_len, query_template, q_name);
		free(q_name);
	} else {
		// prepare query, quote input against sql injections.		
		query_template = "CALL RicercaDipendente('%s', '%s');";
		q_name = db_quote_string(mysql, name, &q_name_len);
		if (q_name == NULL) {
			db_close(mysql);
			return OUT_OF_MEMORY;
		}
		q_surname = db_quote_string(mysql, surname, &q_surname_len);
		if (q_surname == NULL) {
			free(q_name);
			db_close(mysql);
			return OUT_OF_MEMORY;
		}
		query_len =
			strlen(query_template) + q_name_len + q_surname_len + 1;
		query = malloc(query_len);
		if (query == NULL) {
			db_close(mysql);
			free(q_name);
			free(q_surname);
			return OUT_OF_MEMORY;
		}
		memset(query, 0, query_len);
		snprintf(query, query_len, query_template, q_name, q_surname);
		free(q_name);
		free(q_surname);
	}
	// send the query.
	rs = db_query(mysql, query, 2);
	free(query);
	if (rs == NULL) {
		return INVALID_QUERY;
	}
	db_close(mysql);
	res = malloc(sizeof(*res) * rs->no_row);
	if (res == NULL) {
		free_query_result_set(rs, 2);
		return OUT_OF_MEMORY;
	}
	// parse result set.
	for (size_t i = 0; i < rs->no_row; ++i) {
		int status = 0;
		int pos = rs->no_fields * i;
		status = init_worker_contacts(
			&res[i], rs->result_set[pos], rs->result_set[pos + 1],
			rs->result_set[pos + 2], rs->result_set[pos + 3],
			rs->result_set[pos + 4], rs->result_set[pos + 5],
			rs->result_set[pos + 6], rs->result_set[pos + 7]);
		if (status) {
			for (size_t j = 0; j < i; ++j) {
				free_worker_contact(&res[j]);
			}
			free_query_result_set(rs, 2);
			free(res);
			return OUT_OF_MEMORY;
		}
	}
	*no_worker = rs->no_row;
	*result = res;
	free_query_result_set(rs, 2);
	return SUCCESS;
}

int search_office(struct database_conn_param *param, char *phone,
		  struct Office **result)
{
	char *q_phone = NULL;
	size_t q_phone_len = 0;
	char *query_template = NULL;
	char *query = NULL;
	size_t query_len = 0;
	struct query_result_set *rs = NULL;
	MYSQL *mysql = NULL;
	struct Office *office = NULL;
	int office_id = 0;
	const size_t MAX_ID_STRING_SIZE = 10;
	if (param == NULL || phone == NULL || result == NULL) {
		return INVALID_PARAMETER;
	}
	/* Prepare first query.*/
	query_template = "CALL RicercaUfficio('%s');";
	mysql = db_connect(param);
	if (mysql == NULL)
		return LOGIN_FAILED;
	// quote string to avoid sql injection.
	q_phone = db_quote_string(mysql, phone, &q_phone_len);
	if (q_phone == NULL) {
		db_close(mysql);
		return OUT_OF_MEMORY;
	}
	query_len = strlen(query_template) + q_phone_len + 1;
	query = malloc(query_len);
	if (query == NULL) {
		free(q_phone);
		db_close(mysql);
		return OUT_OF_MEMORY;
	}
	memset(query, 0, query_len);
	snprintf(query, query_len, query_template, q_phone);
	free(q_phone);
	/* Execute first query*/
	rs = db_query(mysql, query, 2);
	free(query);
	if (rs == NULL) {
		db_close(mysql);
		return INVALID_QUERY;
	}
	/* if the query return an empty set exit*/
	if (rs->no_row != 1) {
		db_close(mysql);
		return EMPTY_SET_RETURNED;
	}
	/* copy query result set inside struct Office*/
	office = malloc(sizeof(*office));
	if (office == NULL) {
		db_close(mysql);
		return OUT_OF_MEMORY;
	}
	memset(office, 0, sizeof(*office));
	office_id = atoi(rs->result_set[0]);
	office->name = strdup(rs->result_set[1]);
	if (office->name == NULL) {
		db_close(mysql);
		free_query_result_set(rs, 2);
		return OUT_OF_MEMORY;
	}
	office->floor = strdup(rs->result_set[2]);
	if (office->floor == NULL) {
		db_close(mysql);
		free(office->name);
		free_query_result_set(rs, 2);
		return OUT_OF_MEMORY;
	}
	office->building = strdup(rs->result_set[3]);
	if (office->building == NULL) {
		db_close(mysql);
		free(office->name);
		free(office->floor);
		free_query_result_set(rs, 2);
		return OUT_OF_MEMORY;
	}
	free_query_result_set(rs, 2);
	/* prepare second query.*/
	query_template = "CALL ListaDipendentiUfficio('%d');";
	query_len = strlen(query_template) + MAX_ID_STRING_SIZE + 1;
	query = malloc(query_len);
	if (query == NULL) {
		db_close(mysql);
		free(office->name);
		free(office->floor);
		free(office->building);
		return OUT_OF_MEMORY;
	}
	snprintf(query, query_len, query_template, office_id);
	/* call second query. */
	rs = db_query(mysql, query, 2);
	free(query);
	db_close(mysql);
	if (rs == NULL) {
		free(office->name);
		free(office->floor);
		free(office->building);
		return INVALID_QUERY;
	}
	/* copy query result in office struct*/
	office->worker = malloc(sizeof(*office->worker) * rs->no_row);
	if (office->worker == NULL) {
		free(office->name);
		free(office->floor);
		free(office->building);
		free_query_result_set(rs, 2);
		return OUT_OF_MEMORY;
	}
	// parse result set inside output struct.
	memset(office->worker, 0, sizeof(*office->worker) * rs->no_row);
	for (size_t i = 0; i < rs->no_row; ++i) {
		size_t pos = rs->no_fields * i;
		office->worker[i].fiscal_code = strdup(rs->result_set[pos]);
		if (office->worker[i].fiscal_code == NULL) {
			free(office->name);
			free(office->floor);
			free(office->building);
			for (size_t j = 0; j < i; ++j) {
				free(office->worker[j].fiscal_code);
				free(office->worker[j].name);
				free(office->worker[j].surname);
			}
			free(office->worker);
			free(office);
			free_query_result_set(rs, 2);
			return OUT_OF_MEMORY;
		}
		office->worker[i].name = strdup(rs->result_set[pos + 1]);
		if (office->worker[i].name == NULL) {
			free(office->name);
			free(office->floor);
			free(office->building);
			free(office->worker[i].fiscal_code);
			for (size_t j = 0; j < i; ++j) {
				free(office->worker[j].fiscal_code);
				free(office->worker[j].name);
				free(office->worker[j].surname);
			}
			free(office->worker);
			free(office);
			free_query_result_set(rs, 2);
			return OUT_OF_MEMORY;
		}
		office->worker[i].surname = strdup(rs->result_set[pos + 2]);
		if (office->worker[i].surname == NULL) {
			free(office->name);
			free(office->floor);
			free(office->building);
			free(office->worker[i].fiscal_code);
			free(office->worker[i].name);
			for (size_t j = 0; j < i; ++j) {
				free(office->worker[j].fiscal_code);
				free(office->worker[j].name);
				free(office->worker[j].surname);
			}
			free(office->worker);
			free(office);
			free_query_result_set(rs, 2);
			return OUT_OF_MEMORY;
		}
		office->worker[i].task = NULL;
		office->worker[i].transfer = atoi(rs->result_set[pos + 3]);
	}
	office->no_worker = rs->no_row;
	free_query_result_set(rs, 2);
	*result = office;
	return SUCCESS;
}

int list_worker_history(struct database_conn_param *param, char *dip_fc,
			struct WorkerHistory **result, size_t *no_wh)
{
	char *query_template = "CALL StoricoDipendente('%s');";
	char *q_dip_fc = NULL;
	size_t q_dip_fc_len = 0;
	char *query = NULL;
	size_t query_len = 0;
	MYSQL *mysql = NULL;
	struct query_result_set *rs = NULL;
	struct WorkerHistory *wh = NULL;
	if (param == NULL || dip_fc == NULL || result == NULL || no_wh == NULL)
		return INVALID_PARAMETER;
	/* escape input*/
	mysql = db_connect(param);
	if (mysql == NULL) {
		return LOGIN_FAILED;
	}
	q_dip_fc = db_quote_string(mysql, dip_fc, &q_dip_fc_len);
	if (q_dip_fc == NULL) {
		db_close(mysql);
		return OUT_OF_MEMORY;
	}
	query_len = strlen(query_template) + q_dip_fc_len + 1;
	query = malloc(query_len);
	if (query == NULL) {
		free(q_dip_fc);
		db_close(mysql);
		return OUT_OF_MEMORY;
	}
	memset(query, 0, query_len);
	snprintf(query, query_len, query_template, q_dip_fc);
	free(q_dip_fc);
	// send query
	rs = db_query(mysql, query, 2);
	free(query);
	db_close(mysql);
	if (rs == NULL) {
		return INVALID_QUERY;
	}
	wh = malloc(rs[0].no_row * sizeof(*wh));
	if (wh == NULL)
		return OUT_OF_MEMORY;
	// parse result set
	memset(wh, 0, rs[0].no_row * sizeof(*wh));
	for (size_t i = 0; i < rs[0].no_row; ++i) {
		size_t pos = rs[0].no_fields * i;
		wh[i].start_date = strdup(rs[0].result_set[pos]);
		wh[i].end_date = strdup(rs[0].result_set[pos + 1]);
		wh[i].office_phone = strdup(rs[0].result_set[pos + 2]);
		wh[i].office = strdup(rs[0].result_set[pos + 3]);
		wh[i].building = strdup(rs[0].result_set[pos + 4]);
		wh[i].task = strdup(rs[0].result_set[pos + 5]);
		if (!wh[i].start_date || !wh[i].end_date ||
		    !wh[i].office_phone || !wh[i].office || !wh[i].building ||
		    !wh[i].task) {
			for (size_t j = 0; j <= i; ++j) {
				free(wh[j].start_date);
				free(wh[j].end_date);
				free(wh[j].office_phone);
				free(wh[j].office);
				free(wh[j].building);
				free(wh[j].task);
			}
			free(wh);
			free_query_result_set(rs, 2);
			return OUT_OF_MEMORY;
		}
	}
	*no_wh = rs->no_row;
	*result = wh;
	free_query_result_set(rs, 2);
	return SUCCESS;
}

int list_to_transfer_worker(struct database_conn_param *param,
			    struct Worker **result, size_t *no_worker)
{
	char *query = "call ListaDipendentiDaTrasferire();";
	MYSQL *mysql = NULL;
	struct query_result_set *rs = NULL;
	struct Worker *worker = NULL;
	// check input
	if (param == NULL || result == NULL || no_worker == NULL)
		return INVALID_PARAMETER;
	mysql = db_connect(param);
	if (mysql == NULL)
		return LOGIN_FAILED;
	// send query
	rs = db_query(mysql, query, 2);
	db_close(mysql);
	if (rs == NULL)
		return INVALID_QUERY;
	worker = malloc(sizeof(*worker) * rs[0].no_row);
	if (worker == NULL) {
		free_query_result_set(rs, 2);
		return OUT_OF_MEMORY;
	}
	// parse result
	memset(worker, 0, sizeof(*worker) * rs[0].no_row);
	for (size_t i = 0; i < rs[0].no_row; ++i) {
		size_t pos = rs[0].no_fields * i;
		worker[i].fiscal_code = strdup(rs[0].result_set[pos]);
		worker[i].name = strdup(rs[0].result_set[pos + 1]);
		worker[i].surname = strdup(rs[0].result_set[pos + 2]);
		worker[i].task = strdup(rs[0].result_set[pos + 3]);
		worker[i].transfer = 1;
		if (worker[i].fiscal_code == NULL || worker[i].name == NULL ||
		    worker[i].surname == NULL || worker[i].task == NULL) {
			free(worker[i].fiscal_code);
			free(worker[i].name);
			free(worker[i].surname);
			for (size_t j = 0; j < i; ++j) {
				free(worker[j].fiscal_code);
				free(worker[j].name);
				free(worker[j].surname);
			}
			free(worker);
			free_query_result_set(rs, 2);
			return OUT_OF_MEMORY;
		}
	}
	*no_worker = rs->no_row;
	*result = worker;
	free_query_result_set(rs, 2);
	return SUCCESS;
}

int list_office(struct database_conn_param *param, struct Office **office,
		size_t *no_office)
{
	char *query = "CALL ListaUffici();";
	MYSQL *mysql = NULL;
	struct query_result_set *rs = NULL;
	struct Office *result = NULL;
	if (param == NULL || office == NULL || no_office == NULL)
		return INVALID_PARAMETER;
	mysql = db_connect(param);
	if (mysql == NULL)
		return LOGIN_FAILED;
	rs = db_query(mysql, query, 2);
	db_close(mysql);
	if (rs == NULL)
		return INVALID_QUERY;
	result = malloc(sizeof(*result) * rs[0].no_row);
	if (result == NULL) {
		free_query_result_set(rs, 2);
		return OUT_OF_MEMORY;
	}
	// parse result set
	memset(result, 0, sizeof(*result) * rs[0].no_row);
	for (size_t i = 0; i < rs[0].no_row; ++i) {
		int pos = i * rs[0].no_fields;
		result[i].name = strdup(rs[0].result_set[pos]);
		result[i].floor = strdup(rs[0].result_set[pos + 1]);
		result[i].building = strdup(rs[0].result_set[pos + 2]);
		result[i].task = strdup(rs[0].result_set[pos + 3]);
		if (result[i].name == NULL || result[i].floor == NULL ||
		    result[i].building == NULL || result[i].task == NULL) {
			for (size_t j = 0; j <= i; ++j) {
				free(result[j].name);
				free(result[j].floor);
				free(result[j].building);
				free(result[j].task);
			}
			free(result);
			free_query_result_set(rs, 2);
			return OUT_OF_MEMORY;
		}
	}
	*office = result;
	*no_office = rs[0].no_row;
	free_query_result_set(rs, 2);
	return SUCCESS;
}

int list_transfer_position(struct database_conn_param *param, char *worker,
			   struct FreePosition **result, size_t *no_position)
{
	char *query_template = "CALL ListaPostazioniDisponibili('%s');";
	char *q_worker = NULL;
	size_t q_worker_len = 0;
	char *query = NULL;
	size_t query_len = 0;
	MYSQL *mysql = NULL;
	struct query_result_set *rs = NULL;
	struct FreePosition *res = NULL;
	if (param == NULL || worker == NULL || result == NULL ||
	    no_position == NULL)
		return INVALID_PARAMETER;
	// prepare query, escaping output
	mysql = db_connect(param);
	if (mysql == NULL) {
		return LOGIN_FAILED;
	}
	q_worker = db_quote_string(mysql, worker, &q_worker_len);
	if (q_worker == NULL) {
		db_close(mysql);
		return OUT_OF_MEMORY;
	}
	query_len = strlen(query_template) + q_worker_len + 1;
	query = malloc(query_len);
	if (query == NULL) {
		free(q_worker);
		return OUT_OF_MEMORY;
	}
	memset(query, 0, query_len);
	snprintf(query, query_len, query_template, q_worker);
	rs = db_query(mysql, query, 2);
	free(query);
	free(q_worker);
	db_close(mysql);
	if (rs == NULL)
		return INVALID_QUERY;
	res = malloc(sizeof(*res) * rs[0].no_row);
	if (res == NULL) {
		free_query_result_set(rs, 2);
		return OUT_OF_MEMORY;
	}
	// parse query result.
	memset(res, 0, sizeof(*res) * rs[0].no_row);
	for (size_t i = 0; i < rs[0].no_row; ++i) {
		int pos = rs[0].no_fields * i;
		res[i].phone = strdup(rs[0].result_set[pos]);
		res[i].office = strdup(rs[0].result_set[pos + 1]);
		res[i].building = strdup(rs[0].result_set[pos + 2]);
		res[i].worker = strdup(rs[0].result_set[pos + 3]);
		if (res[i].phone == NULL || res[i].office == NULL ||
		    res[i].building == NULL || res[i].worker == NULL) {
			for (size_t j = 0; j <= i; ++j) {
				free(res[j].phone);
				free(res[j].office);
				free(res[j].building);
				free(res[j].worker);
			}
			free(res);
			free_query_result_set(rs, 2);
			return OUT_OF_MEMORY;
		}
	}
	*result = res;
	*no_position = rs[0].no_row;
	free_query_result_set(rs, 2);
	return SUCCESS;
}

int transfer_worker_to_free_position(struct database_conn_param *param,
				     char *dip_fc, char *position)
{
	char *q_dip_fc = NULL;
	size_t q_dip_fc_len = 0;
	char *q_position = NULL;
	size_t q_position_len = 0;
	MYSQL *mysql = NULL;
	char *query_template = "CALL TrasferisciDipendente('%s', '%s');";
	char *query = NULL;
	size_t query_len = 0;
	int res = 0;
	if (param == NULL || dip_fc == NULL || position == NULL)
		return INVALID_PARAMETER;
	// prepare query, escaping output	
	mysql = db_connect(param);
	if (mysql == NULL) {
		return LOGIN_FAILED;
	}
	q_dip_fc = db_quote_string(mysql, dip_fc, &q_dip_fc_len);
	if (q_dip_fc == NULL) {
		db_close(mysql);
		return OUT_OF_MEMORY;
	}
	q_position = db_quote_string(mysql, position, &q_position_len);
	if (q_position == NULL) {
		db_close(mysql);
		return OUT_OF_MEMORY;
	}
	query_len = strlen(query_template) + q_dip_fc_len + q_position_len + 1;
	query = malloc(query_len);
	if (query == NULL) {
		db_close(mysql);
		free(q_dip_fc);
		free(q_position);
		return OUT_OF_MEMORY;
	}
	memset(query, 0, query_len);
	snprintf(query, query_len, query_template, q_dip_fc, q_position);
	
	res = db_statement(mysql, query);
	free(query);
	free(q_dip_fc);
	free(q_position);
	db_close(mysql);
	return (res == 0) ? SUCCESS : INVALID_QUERY;
}

int transfer_worker_swap(struct database_conn_param *param,
			 char *dip_fc1,
			 char *dip_fc2)
{
	char *q_dip_fc1 = NULL;
	size_t q_dip_fc1_len = 0;
	char *q_dip_fc2 = NULL;
	size_t q_dip_fc2_len = 0;
	MYSQL *mysql = NULL;
	char *query_template = "CALL TrasferisciDipendenteSWAP('%s', '%s');";
	char *query = NULL;
	size_t query_len = 0;
	int res = 0;
	if (param == NULL || dip_fc1 == NULL || dip_fc2 == NULL)
		return INVALID_PARAMETER;
	// prepare query, escaping output	
	mysql = db_connect(param);
	if (mysql == NULL) {
		return LOGIN_FAILED;
	}
	q_dip_fc1 = db_quote_string(mysql, dip_fc1, &q_dip_fc1_len);
	if (q_dip_fc1 == NULL) {
		db_close(mysql);
		return OUT_OF_MEMORY;
	}
	q_dip_fc2 = db_quote_string(mysql, dip_fc2, &q_dip_fc2_len);
	if (q_dip_fc2 == NULL) {
		free(q_dip_fc1);
		db_close(mysql);
		return OUT_OF_MEMORY;
	}
	query_len = strlen(query_template) + q_dip_fc1_len + q_dip_fc2_len + 1;
	query = malloc(query_len);
	if (query == NULL) {
		db_close(mysql);
		free(q_dip_fc1);
		free(q_dip_fc2);
		return OUT_OF_MEMORY;
	}
	memset(query, 0, query_len);
	snprintf(query, query_len, query_template, q_dip_fc1, q_dip_fc2);
	free(q_dip_fc1);
	free(q_dip_fc2);
	res = db_statement(mysql, query);
	free(query);
	db_close(mysql);
	return (res == 0) ? SUCCESS : INVALID_QUERY;
}

int update_worker_task(struct database_conn_param *param, char *dip_fc,
		       char *task)
{
	char *q_dip_fc = NULL;
	size_t q_dip_fc_len = 0;
	char *q_task = NULL;
	size_t q_task_len = 0;
	MYSQL *mysql = NULL;
	char *query_template = "CALL ModificaMansione('%s', '%s');";
	char *query = NULL;
	size_t query_len = 0;
	int res = 0;
	if (param == NULL || dip_fc == NULL || task == NULL)
		return INVALID_PARAMETER;
	// prepare query, escaping output	
	mysql = db_connect(param);
	if (mysql == NULL) {
		return LOGIN_FAILED;
	}
	q_dip_fc = db_quote_string(mysql, dip_fc, &q_dip_fc_len);
	if (q_dip_fc == NULL) {
		db_close(mysql);
		return OUT_OF_MEMORY;
	}
	q_task = db_quote_string(mysql, task, &q_task_len);
	if (q_task == NULL) {
		db_close(mysql);
		free(q_dip_fc);
		return OUT_OF_MEMORY;
	}
	query_len = strlen(query_template) + q_dip_fc_len + q_task_len + 1;
	query = malloc(query_len);
	if (query == NULL) {
		db_close(mysql);
		free(q_dip_fc);
		free(q_task);
		return OUT_OF_MEMORY;
	}
	memset(query, 0, query_len);
	snprintf(query, query_len, query_template, q_dip_fc, q_task);
	free(q_dip_fc);
	free(q_task);
	res = db_statement(mysql, query);
	free(query);
	db_close(mysql);
	return (res == 0) ? SUCCESS : INVALID_QUERY;
}
