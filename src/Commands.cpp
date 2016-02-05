#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "Commands.h"
#include "CHidApi.h"
#include "RFIDDB.h"

#define TRACE

// Mutex to avoid multiple commands altogether. Actually the management
// is not multithreaded from a client POV, but it will eventually turned to be
static pthread_mutex_t sCmdMutex;
// Threads repository
static vector<pthread_t> sThreads;

#define STRLENGTH				256		// String built from the RFID, made by 255 + 1 (tail char, i.e. 0x00)

static int sThreadId = 0;

static void* ThreadCbk(void *pPtr)
{
	CHidApi *pHID = (CHidApi *)pPtr;
	CRFIDDB DBConn;
	char	RFID[STRLENGTH], Time[STRLENGTH];
	int		i, Count = 0, ThreadId;
	
	sThreadId++;
	ThreadId = sThreadId;
	printf("Thread %d started, pPtr=%p, HID=%p\n", ThreadId, pHID, pHID->GetHandle());
	// Open a connection onto the RFIDDB
	if (!DBConn.Connect(RFIDDB))
	{
		printf("ERROR: cannot open a connection on RFIDDB: Exiting Thread");
		return NULL;
	}
	time_t t;
	struct tm tm;
	// Read a buffer report here. Internally it uses a hid_read
	// which put the thread in sleep until something is ready to be read
	while (pHID->ReadReport())
	{
#ifdef TRACE
		printf("Thread %d: ReadReport()\n", ThreadId);
#endif
		//printf("Thread ID %d\n", ThreadId);
		// Loop on pHid buffer and paste each valid (i.e. != 0) byte to the RFID
		for (i = 0; i < USB_BUFFER_DIM; ++i)
		{
			// WARNING: since the RFID reader is treated as a keyboard, and we
			// can have many scancode without a unique way to translate them,
			// we just keep numbers (30 -> 39)and CR (40), discarding all other keys
			if (pHID->m_USB_RxData[i] >= 30 && pHID->m_USB_RxData[i] <= 40)
			{
#ifdef TRACE
				printf("%c", pHID->m_USB_RxData[i]);
#endif
				// If the code is 40dec, the reader just returned a CR, so close current RFID
				switch (pHID->m_USB_RxData[i])
				{
				case 40: // CR
					RFID[Count] = 0x00;
					// Dump the RFID
					//printf("%s (ThreadId = %d, Handle=%p)\n", RFID, ThreadId, pHID->GetHandle());
					// Reset the Counter initing a new RFID string, eventually
					Count = 0;
					// Get current time
					t = time(NULL);
					tm = *localtime(&t);
					sprintf(Time, "%d-%d-%d %d:%d:%d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
					// Now we have a well cooked RFID ready to be pushed to the RFIDDB
					DBConn.AddTag(ThreadId, RFID, Time);
#ifdef TRACE
					printf("\nThread %d: Tag Added\n", ThreadId);
#endif
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
	printf("Exiting Thread %d\n", ThreadId);
	
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
	printf("HidHandles.Size()==%d\n", HidHandles.size());
	// Loop on them creating one thread per handle
	vector<CHidApi>::const_iterator const_itor;
	for (const_itor = HidHandles.begin(); const_itor != HidHandles.end(); ++const_itor)
	{
		pthread_t Thread;
		// Gives current HidHandle as thread cbk parameter
		const CHidApi *pPtr = &*const_itor;
		printf("HidApi * p = %p, pHID=%p\n", pPtr, pPtr->GetHandle());
		if (!pthread_create(&Thread, NULL, ThreadCbk, (void*)(&*const_itor)))
			sThreads.push_back(Thread);
		else
			printf("ERROR: Fail to create listener Thread!!!");
	}
}

void GetXMLSnapShot(char XMLSnapShot[])
{ 
	strcpy(XMLSnapShot, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	strcat(XMLSnapShot, "<SnapShot>");
		// Loop on TAG table entries and fill the XML. Use the already open connection to do so.
	if (!MainConnect.SelectFromTable("TAG"))
		return;
	vector<string> Fields;
	while (MainConnect.GetRowStrings(Fields))
//	for(int i=1; i<=2; ++i)
	{
		strcat(XMLSnapShot, "<Tag");
		// Write the Reader Id as attribute
		strcat(XMLSnapShot, " R=\"");
		strcat(XMLSnapShot, Fields[0].c_str());
		strcat(XMLSnapShot, "\"");
		// Write the tag Id as attribute
		strcat(XMLSnapShot, " Id=\"");
		strcat(XMLSnapShot, Fields[1].c_str());
		strcat(XMLSnapShot, "\"");
		// Write the logging time as attribute
		strcat(XMLSnapShot, " T=\"");
		strcat(XMLSnapShot, Fields[2].c_str());
		strcat(XMLSnapShot, "\">");
		strcat(XMLSnapShot, "</Tag>");
	}
	// Add a dummy fps entry, just to test the refresh speed
	static int Count = 1;
	char String[256];
	sprintf(String, "<fps>%d</fps>", Count++);
	strcat(XMLSnapShot, String);
	// Close the root
	strcat(XMLSnapShot, "</SnapShot>\n");
	//printf(XMLSnapShot);
}

void CommandDispatcher(char XMLSnapShot[], const char Cmd[])
{
	// These mutex could be avoided, since we are using the microwebserver
	// not in multithreaded mode, so each connection is served sequentially by a queue
	// but we leave it here to support the webserver in multithreaded mode
	pthread_mutex_lock(&sCmdMutex);
	// Switch among commands
	if ( !strcmp(Cmd, "Recog") )
		CmdRecognize();
	
	GetXMLSnapShot(XMLSnapShot);	
	
	// Finally fill the XML and return it
//	FillXMLFromMasters(XMLSnapShot);

	pthread_mutex_unlock(&sCmdMutex);
}

