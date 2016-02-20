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
	bool EmptyTable		 (const char TableName[]);
	void Close			 (void);
};


#endif