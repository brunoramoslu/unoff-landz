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

#ifndef DB_ATTRIBUTE_TBL_H_INCLUDED
#define DB_ATTRIBUTE_TBL_H_INCLUDED

#define ATTRIBUTE_TABLE_SQL "CREATE TABLE ATTRIBUTE_TABLE( \
        ATTRIBUTE_ID  INTEGER PRIMARY KEY        NOT NULL,  \
        RACE_ID             INT, \
        ATTRIBUTE_TYPE_ID   INT, \
        PICKPOINTS          INT, \
        ATTRIBUTE_VALUE     INT)"

/** RESULT   : loads attributes and associated attribute values from the database to memory array

    RETURNS  : void

    PURPOSE  : retrieves attribute/attribute value data from permanent storage

    NOTES    :
**/
void load_db_attributes();


/** RESULT  : loads attribute data specified in a text file

    RETURNS : void

    PURPOSE : batch loading of attribute data

    NOTES   :
*/
void batch_add_attributes(char *file_name, int attribute_type_id);


/** RESULT  : removes a specific attribute type from the database

    RETURNS : void

    PURPOSE : allows updating of attribute profiles

    NOTES   :
*/
void delete_db_attribute(int attribute_type_id);

#endif // DB_ATTRIBUTE_TBL_H_INCLUDED
