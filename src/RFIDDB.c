#include "RFIDDB.h"
#include <my_global.h>
#include <mysql.h>

static MYSQL *con = NULL; 

static void finish_with_error(MYSQL *con)
{
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);        
}


void MySQLInit(void)
{
	printf("MySQL client version: %s\n", mysql_get_client_info());
	
	con = mysql_init(NULL);
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
	 if (mysql_query(con, "CREATE DATABASE IF NOT EXISTS RFIDDB")) 
		  finish_with_error(con);
	 if (mysql_query(con, "USE RFIDDB")) 
		  finish_with_error(con);
	 // Always drop the TAG table
     if (mysql_query(con, "DROP TABLE IF EXISTS TAG"))
         finish_with_error(con);
	 // Create a new one
     if (mysql_query(con, "CREATE TABLE TAG(Id INT, Tag TEXT, Time TEXT)"))
         finish_with_error(con);	 
}

void MySQLAddTag(int Reader, char Tag[])
{
	char String[256];
	
	sprintf(String, "INSERT INTO TAG VALUES(%d,%s,%s)", Reader, Tag, "");
	mysql_query(con, String);
}





