/*
     This file is part of HIDWebServer
     (C) Riccardo Ventrella
     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 3.0 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "RFIDDB.h"
#include <my_global.h>
#include <mysql.h>


static void finish_with_error(MYSQL *con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);        
}


void CRFIDDB::CreateDBAndTable(const char DBName[])
{
	printf("MySQL client version: %s\n", mysql_get_client_info());
	
	MYSQL* con = mysql_init(NULL);
	  if (con == NULL) 
	  {
	      fprintf(stderr, "%s\n", mysql_error(con));
	      exit(1);
	  }
	  // Connect to 127.0.0.1 to overcome problem with Unix socket permissions on OsX.
	  // Using this host, the connection goes through TCP/IP which is a bit slower
	  // but avoids to have problems with Unix sockets.
	  if (mysql_real_connect(con, "127.0.0.1", "root", "gandalf", NULL, 0, NULL, 0) == NULL) 
		  finish_with_error(con);
	  // Check the RFIDDataBase already exists or eventually create a new one
	  sprintf(m_String, "CREATE DATABASE IF NOT EXISTS %s", DBName);
	 if (mysql_query(con, m_String)) 
		  finish_with_error(con);
	  sprintf(m_String, "USE %s", DBName);
	 if (mysql_query(con, m_String)) 
		  finish_with_error(con);
	 // Always drop the TAG table
     if (mysql_query(con, "DROP TABLE IF EXISTS TAG"))
         finish_with_error(con);
	 // Create a new one
     if (mysql_query(con, "CREATE TABLE TAG(Id INT, Tag TEXT, Time TEXT)"))
         finish_with_error(con);	 
	 // Finally store it as void* within the class
	 m_pCon = reinterpret_cast<void*>(con);
}

bool CRFIDDB::Connect(const char DBName[])
{
	MYSQL* con = mysql_init(NULL);
	if (con == NULL) 
	{
	   fprintf(stderr, "%s\n", mysql_error(con));
	   return false;
	}	
  // Connect to 127.0.0.1 to overcome problem with Unix socket permissions on OsX.
  // Using this host, the connection goes through TCP/IP which is a bit slower
  // but avoids to have problems with Unix sockets.
  if (mysql_real_connect(con, "127.0.0.1", "root", "gandalf", DBName, 0, NULL, 0) == NULL)
	  return false;
	 // Finally store it as void* within the class
	m_pCon = reinterpret_cast<void*>(con);
	 
	 return true;
}

void CRFIDDB::AddTag(int Reader, const char Tag[], const char Time[])
{
	if (!m_pCon)
		return;
	sprintf(m_String, "INSERT INTO TAG VALUES(%d,'%s','%s')", Reader, Tag, Time);
	if(mysql_query(reinterpret_cast<MYSQL*>(m_pCon), m_String))
	{
		printf("Error on INSERT query");
		return;
	}
	
}

bool CRFIDDB::SelectFromTable(const char TableName[])
{
	if (!m_pCon)
		return false;
	sprintf(m_String, "SELECT * FROM %s", TableName);
	if (mysql_query(reinterpret_cast<MYSQL*>(m_pCon), m_String))
		return false;
	MYSQL_RES *result = mysql_store_result(reinterpret_cast<MYSQL*>(m_pCon));
	if (result == NULL)
		return false;
	m_nFields = mysql_num_fields(result);
	m_pResult = reinterpret_cast<void*>(result);
	return true;
}

// This should be done after a SelectFromTable call, which prepare the query result
bool CRFIDDB::GetRowStrings(vector<string>& Strings)
{
	if (!m_pResult || !m_nFields)
		return false;
	MYSQL_ROW row;
	if (!(row = mysql_fetch_row(reinterpret_cast<MYSQL_RES*>(m_pResult))))
		return false;
	// There's no need to re-allocate the vector, if already full (avoid to waste memory and time)
	if(Strings.empty())
	{
		for (int i = 0; i < m_nFields; ++i)
		{
			std::string String(row[i]);
			Strings.push_back(String);
		}
		
	}
	else
		for (int i = 0; i < m_nFields; ++i)
			Strings[i] = row[i];
	return true;
}

bool CRFIDDB::EmptyTable(const char TableName[])
{
	if (!m_pCon)
		return false;
	sprintf(m_String, "TRUNCATE TABLE %s", TableName);
	if (mysql_query(reinterpret_cast<MYSQL*>(m_pCon), m_String))
		return false;
	return true;
}

void CRFIDDB::Close(void)
{
	if (m_pCon)
		mysql_close(reinterpret_cast<MYSQL*>(m_pCon));
}



