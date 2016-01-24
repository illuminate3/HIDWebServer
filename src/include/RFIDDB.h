#ifndef __RFIDDB_H__
#define __RFIDDB_H__
#include <stddef.h>

class CRFIDDB
{
	void *m_pCon;		// We use a void* for the connection to uncouple the DB engine from the client
	char m_String[256];
		
public:
	// Ctor
	CRFIDDB(void) : m_pCon(NULL)
	{
		m_String[0] = 0;
	}
		
	void CreateDBAndTable	(const char DBName[]);
	bool Connect			(const char DBName[]);
	void AddTag				(int Reader, char Tag[]);	
};


#endif