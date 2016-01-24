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

void CRFIDDB::AddTag(int Reader, char Tag[])
{
	sprintf(m_String, "INSERT INTO TAG VALUES(%d,%s,%s)", Reader, Tag, "");
	mysql_query(reinterpret_cast<MYSQL*>(m_pCon), m_String);
}





