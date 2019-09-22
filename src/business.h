#ifndef BUSINESS_H
#define BUSINESS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "database.h"

#define SUCCESS 0
#define INVALID_PARAMETER 1
#define OUT_OF_MEMORY 2
#define LOGIN_FAILED 3
#define INVALID_QUERY 4
#define EMPTY_SET_RETURNED 5

static char *errmsg[] = {"SUCCESS",
			 "INVALID PARAMETER",
			 "OUT OF MEMORY",
			 "LOGIN FAILED",
			 "INVALID QUERY",
			 "EMPTY SET RETURNED"};

struct Worker {
	char *fiscal_code;
	char *name;
	char *surname;
	char *task;
	int transfer;
};

struct Worker_Contacts {
	char *fiscal_code;
	char *name;
	char *surname;
	char *email;
	char *office_email;
	char *office_phone;
	char *office;
	char *building;
};

struct Office {
	char *name;
	char *floor;
	char *building;
	char *task;
	size_t no_worker;
	struct Worker *worker;
};

struct WorkerHistory {
	char *start_date;
	char *end_date;
	char *office_phone;
	char *office;
	char *building;
	char *task;
};

struct FreePosition{
	char *phone;
	char *office;
	char *building;
	char *worker;
};

/**
 * get_error() - return an error message.
 * @errnum: code returned by the function defined in this header.
 */
char *get_error(int errnum);

/**
 * search_worker() - search worker contacts given the name or surname of said worker.
 * @param: database connection parameters.
 * @name: name of the worker to search.
 * @surname: surname of the worker to search.
 * @result: pointer to where the result should be put.
 * @no_worker: pointer to a variable that hold how many worker have been found.
 *
 * search_worker() will research a worker given his/her name or surname.
 * If more worker have the same name, or the same surname more than
 * one result will be returned.
 * The result will be given as an malloc() allocated array of `Worker_Contacts`
 * put in @result, whose size is given in @no_worker.
 *
 * Return: 0 in case of success, an error code in case of failure.
 */
int search_worker(struct database_conn_param *param,
		  char *name,
		  char *surname,
		  struct Worker_Contacts **result,
		  size_t *no_worker);

/**
 * search_office() - return all the info about an office.
 * @param: database connection parameters.
 * @phone: phone number of a postation inside the office.
 * @result: pointer to where the result is stored.
 *
 * search_office() will search of the info about an office
 * and the worker that work inside it.
 * the @result is an malloc() allocated, this function
 * can return only one office.
 * 
 * Return: 0 in case of success, an error code in case of failure.
 */
int search_office(struct database_conn_param *param,
		  char *phone,
		  struct Office **result);

/**
 * list_worker_history() - return a worker work's histories.
 * @param: database connection parameters.
 * @dip_fc: fiscal code of the worker.
 * @result: pointer to where the result is stored.
 * @no_wh: size of the result.
 * 
 * list_worker_history() will return a list of all
 * the worker working place.
 *
 * Return: 0 in case of success, an error code in case of failure.
 */
int list_worker_history(struct database_conn_param *param,
			char *dip_fc,
			struct WorkerHistory **result,
			size_t *no_wh);

/**
 * list_to_transfer_worker() - return a list of all worker to transfer.
 * @param: database connection parameters.
 * @result: pointer to where the result is stored.
 * @no_worker: size of the result.
 *
 * list_to_transfer_worker() will return a list of all worker
 * that need to be transfered.
 *
 * Return: 0 in case of success, an error code in case of failure.
 *
 */
int list_to_transfer_worker(struct database_conn_param *param,
			    struct Worker **result,
			    size_t *no_worker);


/**
 * list_office() - list all offices.
 * @param: database connection parameters.
 * @office: pointer to where to store the result.
 * @no_office: size of the result.
 *
 * Return: 0 in case of success, an error code in case of failure.
 *
 */
int list_office(struct database_conn_param *param,
		struct Office **office,
		size_t *no_office);

/**
 * list_transfer_position() - list all position where a worker could be transfered in.
 * @param: database connection parameters.
 * @dip_fc: worker fiscal code.
 * @result: pointer to where to store the result.
 * @no_position: size of the result.
 *
 * list_transfer_position() will return a list of position where the worker
 * could be trasferred in.
 *
 * Return: 0 in case of success, an error code in case of failure.
 *
 */
int list_transfer_position(struct database_conn_param *param,
			   char *dip_fc,
			   struct FreePosition **result,
			   size_t *no_position);

/**
 * transfer_worker_to_free_position() - change a worker position to a free position.
 * @param: database connection parameters.
 * @dip_fc: worker fiscal code.
 * @position: free position phone number.
 *
 * transfer_worker_to_free_position() will transfer a worker to a new
 * free position indicated by the phone number @position. 
 *
 * Return: 0 in case of success, an error code in case of failure.
 *
 */
int transfer_worker_to_free_position(struct database_conn_param *param,
				     char *dip_fc,
				     char *position);
/**
 * transfer_worker_swap() - swap worker position.
 * @param: database connection parameters.
 * @dip_fc_1: fiscal code of the first worker.
 * @dip_fc_2: fiscal code of the second worker.
 *
 * transfer_worker_swap() will swap the position of two worker that
 * have the same task and need to be trasferred.
 *
 * Return: 0 in case of success, an error code in case of failure.
 *
 */
int transfer_worker_swap(struct database_conn_param *param,
			 char *dip_fc_1,
			 char *dip_fc_2);
/**
 * update_worker_task() - update the task of worker.
 * @param: database connection parameters.
 * @dip_fc: fiscal code of the worker.
 * @task: new task.
 * 
 * update_worker_task() will update the task of a worker
 * and update his transfer state.
 *
 * Return: 0 in case of success, an error code in case of failure.
 *
 */
int update_worker_task(struct database_conn_param *param,
		       char *dip_fc,
		       char *task);

#endif
