#ifndef __RFIDDB_H__
#define __RFIDDB_H__
#include <stddef.h>
#include <vector>
#include <string>

using namespace std;


// Database name
#define RFIDDB		"RFIDDB"

class CRFIDDB
{
	void *m_pCon;		// We use a void* for the connection to uncouple the DB engine from the client
	void *m_pResult;
	char  m_String[256];
	int	  m_nFields;
		
public:
	// Ctor
	CRFIDDB(void) : m_pCon(NULL), m_pResult(NULL), m_nFields(0)
	{
		m_String[0] = 0;
	}
		
	void CreateDBAndTable(const char DBName[]);
	bool Connect		 (const char DBName[]);
	void AddTag			 (int Reader, const char Tag[], const char Time[]);
	bool SelectFromTable (const char TableName[]);
	bool GetRowStrings	 (vector<string>& Strings);
	void Close			 (void);
};


#endif