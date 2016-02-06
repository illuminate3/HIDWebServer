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
