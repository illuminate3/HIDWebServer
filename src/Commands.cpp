#include <string.h>
#include <pthread.h>
#include "Commands.h"
#include "CHidApi.h"
#include "RFIDDB.h"

// Mutex to avoid multiple commands altogether. Actually the management
// is not multithreaded from a client POV, but it will eventually turned to be
static pthread_mutex_t sCmdMutex;
// Threads repository
static vector<pthread_t> sThreads;

#define RFIDLENGTH				256		// String built from the RFID, made by 255 + 1 (tail char, i.e. 0x00)

static int sThreadId = 0;

static void* ThreadCbk(void *pPtr)
{
	CHidApi *pHID = (CHidApi *)pPtr;
	CRFIDDB DBConn;
	char	RFID[RFIDLENGTH];
	int		i, Count = 0, ThreadId;
	
	sThreadId++;
	ThreadId = sThreadId;
	printf("Thread %d started, HID=%p\n", ThreadId, pHID->GetHandle());
	// Open a connection onto the RFIDDB
	if (!DBConn.Connect(RFIDDB))
	{
		printf("ERROR: cannot open a connection on RFIDDB: Exiting Thread");
		return NULL;
	}
	// Read a buffer report here. Internally it uses a hid_read
	// which put the thread in sleep until something is ready to be read
	while (pHID->ReadReport())
	{
		//printf("Thread ID %d\n", ThreadId);
		// Loop on pHid buffer and paste each valid (i.e. != 0) byte to the RFID
		for (i = 0; i < USB_BUFFER_DIM; ++i)
		{
			// WARNING: since the RFID reader is treated as a keyboard, and we
			// can have many scancode without a unique way to translate them,
			// we just keep numbers (30 -> 39)and CR (40), discarding all other keys
			if (pHID->m_USB_RxData[i] >= 30 && pHID->m_USB_RxData[i] <= 40)
			{
				// If the code is 40dec, the reader just returned a CR, so close current RFID
				switch (pHID->m_USB_RxData[i])
				{
				case 40: // CR
					RFID[Count] = 0x00;
					// Dump the RFID
					printf("%s (ThreadId = %d, Handle=%p)\n", RFID, ThreadId, pHID->GetHandle());
					// Reset the Counter initing a new RFID string, eventually
					Count = 0;
					// Now we have a well cooked RFID ready to be pushed to the RFIDDB
					DBConn.AddTag(ThreadId, RFID);
					break;
				// Put this char within the buffer, after having translated it.
				// According to hid keyboards, we have '1' == 30, ..., '9' == 38 and '0' == 39
				case 39: // '0'
					RFID[Count++] = 48;
					break;
				default: // 1 -> 9
					RFID[Count++] = pHID->m_USB_RxData[i] + 19;
					break;
				}
			}
		}
	}
	
	DBConn.Close();
	printf("Exiting Thread");
	
	return NULL;
}

// This the connection to the DB we use in the main thread.
// The connection is kept opened in order to enumerate the TAG table
// content and deply it to the HTML page via an XML
static CRFIDDB MainConnect;

void CommandInit(void)
{
	pthread_mutex_init(&sCmdMutex, NULL);
	// Init the DBStuff as well
	MainConnect.CreateDBAndTable("RFIDDB");	
}


void CommandQuit(void)
{
	pthread_mutex_destroy(&sCmdMutex);
}


void CmdRecognize(void)
{
	// If the are threads running, first we have to signal them to kill themselves
	// and the wait them to join before going on.
	if (sThreads.size())
	{

		sThreads.empty();
	}
	vector<CHidApi> HidHandles;

	// Recognize all RFID readers and launch a thread for each of them to do parallel reading
	CHidApi::FindRFIDReadersHids(HidHandles);
	// Loop on them creating one thread per handle
	vector<CHidApi>::const_iterator const_itor;
	for (const_itor = HidHandles.begin(); const_itor != HidHandles.end(); ++const_itor)
	{
		pthread_t Thread;
		// Gives current HidHandle as thread cbk parameter
		if (!pthread_create(&Thread, NULL, ThreadCbk, (void*)&(*const_itor)))
			sThreads.push_back(Thread);
		else
			printf("ERROR: Fail to create listener Thread!!!");
	}
}





/*
void FillXMLFromMasters(char XMLSnapShot[])
{ 
	char Number[16];
	
	strcpy(XMLSnapShot, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	strcat(XMLSnapShot, "<SnapShot>");
	// Loop on masters and fill the xml
	vector<CMaster>::const_iterator	const_itor;
	CSlaves::const_iterator 		Slaves_const_itor;
	CPins::const_iterator			Pins_const_itor;
	// Loop on Master entries
	for ( const_itor = Masters.begin(); const_itor != Masters.end(); ++const_itor )
	{
		strcat(XMLSnapShot, "<Master");
		// Write Master Name as attribute
		strcat(XMLSnapShot, " Name=\"");
		strcat(XMLSnapShot, const_itor->GetName().c_str());
		strcat(XMLSnapShot, "\"");
		// Write Master Id as attribute
		strcat(XMLSnapShot, " Id=\"");
		sprintf(Number, "%d", const_itor->GetId());
		strcat(XMLSnapShot, Number);
		strcat(XMLSnapShot, "\">");
		// Loop on Slaves entries
		const CSlaves& Slvs = const_itor->GetSlaves();
		for ( Slaves_const_itor = Slvs.begin(); Slaves_const_itor != Slvs.end(); ++Slaves_const_itor )
		{
			strcat(XMLSnapShot, "<Slave");
			// Write Slave type as attribute
			strcat(XMLSnapShot, " Type=\"");
			strcat(XMLSnapShot, (*Slaves_const_itor)->GetTypeString());
			strcat(XMLSnapShot, "\"");
			// Write Master Id as attribute
			strcat(XMLSnapShot, " Id=\"");
			sprintf(Number, "%d", (*Slaves_const_itor)->GetId());
			strcat(XMLSnapShot, Number);
			strcat(XMLSnapShot, "\">");
			// Loop on Pins entries
			const CPins& Pins = (*Slaves_const_itor)->GetPins();
			for ( Pins_const_itor = Pins.begin(); Pins_const_itor != Pins.end(); ++Pins_const_itor )
			{
				strcat(XMLSnapShot, "<Pin");
				// Write Pin type as attribute
				strcat(XMLSnapShot, " Type=\"");
				strcat(XMLSnapShot, Pins_const_itor->GetTypeString());
				strcat(XMLSnapShot, "\"");
				// Write Master Id as attribute
				strcat(XMLSnapShot, " Id=\"");
				sprintf(Number, "%d", Pins_const_itor->GetId());
				strcat(XMLSnapShot, Number);
				strcat(XMLSnapShot, "\"></Pin>");
			}
			strcat(XMLSnapShot, "</Slave>");			
		}
		strcat(XMLSnapShot, "</Master>");
	}
	// Add a dummy fps entry, just to test the refresh speed
	static int Count = 1;
	char String[256];
	sprintf(String, "<fps>%d</fps>", Count++);
	strcat(XMLSnapShot, String);
	// Close the root
	strcat(XMLSnapShot, "</SnapShot>\n");
}
*/






void CommandDispatcher(char XMLSnapShot[], const char Cmd[])
{
	pthread_mutex_lock(&sCmdMutex);
	// Switch among commands
	if ( !strcmp(Cmd, "Recog") )
		CmdRecognize();
	
	
	
	// Finally fill the XML and return it
//	FillXMLFromMasters(XMLSnapShot);

	pthread_mutex_unlock(&sCmdMutex);
}

