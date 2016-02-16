/******************************************************************************************************************
	Copyright 2014, 2015, 2016 UnoffLandz

	This file is part of unoff_server_4.

	unoff_server_4 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	unoff_server_4 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with unoff_server_4.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************************************************/

#include <stdio.h> //support for snprintf
#include <stdlib.h> //support for NULL data type and atoi function
#include <string.h> //support for memset function

#include "database_functions.h"
#include "../logging.h"
#include "../character_type.h"
#include "../server_start_stop.h"
#include "../character_race.h"
#include "../gender.h"
#include "../string_functions.h"
#include "db_character_race_tbl.h"
#include "db_gender_tbl.h"


void load_db_char_types(){

    /** public function - see header */

    log_event(EVENT_INITIALISATION, "loading character types...");

    //check database is open and table exists
    check_db_open(GET_CALL_INFO);
    check_table_exists("CHARACTER_TYPE_TABLE", GET_CALL_INFO);

    //check database table exists
    const char database_table[]="CHARACTER_TYPE_TABLE";
    if(table_exists(database_table)==false){

        log_event(EVENT_ERROR, "table [%s] not found in database", database_table);
        stop_server();
    }

    sqlite3_stmt *stmt;

    char sql[MAX_SQL_LEN]="SELECT * FROM CHARACTER_TYPE_TABLE";

    //prepare sql statement
    int rc=sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if(rc!=SQLITE_OK){

        log_sqlite_error("sqlite3_prepare_v2 failed", __func__, __FILE__, __LINE__, rc, sql);
    }

    //read the sql query result into the char type array
    int i=0;
    while ( (rc = sqlite3_step(stmt)) == SQLITE_ROW) {

        int char_type_id=sqlite3_column_int(stmt, 0);

        if(char_type_id>MAX_CHARACTER_TYPES) {

            log_event(EVENT_ERROR, "character_type_id [%i] exceeds max [%i] in function %s: module %s: line %i", char_type_id, MAX_CHARACTER_TYPES, __func__, __FILE__, __LINE__);
            stop_server();
        }

        character_type[char_type_id].race_id=sqlite3_column_int(stmt, 2);
        character_type[char_type_id].gender_id=sqlite3_column_int(stmt, 3);

        log_event(EVENT_INITIALISATION, "loaded [%i] %s %s", char_type_id, race[character_type[char_type_id].race_id].race_name, gender[character_type[char_type_id].gender_id].gender_name);

        i++;
    }

    if (rc != SQLITE_DONE) {

        log_sqlite_error("sqlite3_step failed", __func__, __FILE__, __LINE__, rc, sql);
    }

    //destroy the prepared sql statement
    rc=sqlite3_finalize(stmt);
    if (rc != SQLITE_OK) {

        log_sqlite_error("sqlite3_finalize failed", __func__, __FILE__, __LINE__, rc, sql);
    }

    if(i==0){

        log_event(EVENT_ERROR, "no character types found in database", i);
        stop_server();
    }
}


void add_db_char_type(int char_type_id, int race_id, int gender_id){

    /** RESULT  : loads an entry to the character type table

        RETURNS : void

        PURPOSE : used in batch_add_char_types

        NOTES   :
    **/

    //check database is open and table exists
    check_db_open(GET_CALL_INFO);
    check_table_exists("CHARACTER_TYPE_TABLE", GET_CALL_INFO);

    char sql[MAX_SQL_LEN]="";
    snprintf(sql, MAX_SQL_LEN,
        "INSERT INTO CHARACTER_TYPE_TABLE("  \
        "CHARACTER_TYPE_ID," \
        "RACE_ID," \
        "GENDER_ID" \
        ") VALUES(%i, %i, %i)", char_type_id, race_id, gender_id);

    process_sql(sql);

    //load race and gender data so that we can create a meaningful messages
    load_db_char_races();
    load_db_genders();

    fprintf(stderr, "Character type [%i] gender [%s] race [%s] added successfully\n", char_type_id, race[race_id].race_name, gender[gender_id].gender_name);

    log_event(EVENT_SESSION, "Added character type [%i] gender [%s] race [%s] to CHARACTER_TYPE_TABLE", char_type_id,race[race_id].race_name, gender[gender_id].gender_name);
}


void batch_add_char_types(char *file_name){

    /** public function - see header */

    FILE* file;

    if((file=fopen(file_name, "r"))==NULL){

        log_event(EVENT_ERROR, "char type list file [%s] not found", file_name);
        stop_server();
    }

    char line[160]="";
    int line_counter=0;

    log_event(EVENT_INITIALISATION, "\nAdding char types specified in file [%s]", file_name);
    fprintf(stderr, "\nAdding char types specified in file [%s]\n", file_name);

    while (fgets(line, sizeof(line), file)) {

        line_counter++;

        sscanf(line, "%*s");

        char output[4][80];
        memset(&output, 0, sizeof(output));
        parse_line(line, output);

        add_db_char_type(atoi(output[0]), atoi(output[1]), atoi(output[2]));
    }

    fclose(file);
}
