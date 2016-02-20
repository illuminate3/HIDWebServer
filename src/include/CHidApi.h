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
#ifndef __CHIDAPI_H__
#define __CHIDAPI_H__

#include <memory.h>
#include "hidapi/hidapi.h"

#include <vector>
using namespace std;


#define	USB_BUFFER_DIM		128

class CHidApi
{
	hid_device 	  *m_pHandle;
	
public:
	unsigned char m_USB_RxData[USB_BUFFER_DIM];

	// Ctor
	CHidApi(void) : m_pHandle(NULL)	
	{
		this->ResetRxData();
	}
	
	void ResetRxData(void) { memset(m_USB_RxData, 0x00, USB_BUFFER_DIM*sizeof(unsigned char)); }
	
	void SetHandle		(hid_device *pHandle)	{ m_pHandle = pHandle; }
	const hid_device* GetHandle(void) const { return m_pHandle; }
	bool ReadReport	(void);
	
	static size_t FindRFIDReadersHids(vector<hid_device*>& Handles);
};






#endif
