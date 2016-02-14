#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>
#include "Commands.h"
#include "CHidApi.h"
#include "RFIDDB.h"

//#define TRACE

// Mutex to avoid multiple commands altogether. Actually the management
// is not multithreaded from a client POV, but it will eventually turned to be
static pthread_mutex_t sCmdMutex;
// Threads repository
static vector<pthread_t> sThreads;
// Handle repository
static vector<hid_device*> sHidHandles;

#define STRLENGTH				256		// String built from the RFID, made by 255 + 1 (tail char, i.e. 0x00)

static int sThreadId = 0;

// Handler cought by the threads make it suicide
static void handler(int signum)
{
	printf("Thread killed\n");
	pthread_exit(NULL);
}

static void* ThreadCbk(void *pPtr)
{
	hid_device *pHID = 	(hid_device *)pPtr;
	CRFIDDB 			DBConn;
	CHidApi 			HidApi;
	char				RFID[STRLENGTH], Time[STRLENGTH];
	int					i, Count = 0, ThreadId;
	
	HidApi.SetHandle(pHID);
	
	sThreadId++;
	ThreadId = sThreadId;
	printf("Thread %d started, pHID=%p\n", ThreadId, pHID);
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
	while (HidApi.ReadReport())
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
			if (HidApi.m_USB_RxData[i] >= 30 && HidApi.m_USB_RxData[i] <= 40)
			{
#ifdef TRACE
				printf("%c", HidApi.m_USB_RxData[i]);
#endif
				// If the code is 40dec, the reader just returned a CR, so close current RFID
				switch (HidApi.m_USB_RxData[i])
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
					RFID[Count++] = HidApi.m_USB_RxData[i] + 19;
					break;
				}
			}
		}
	}
	
	// This happens when a reader undergoes to a hot unplugging
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
	// Set the handler for managing signal to thread
	signal(SIGUSR1, handler);
}


void CommandQuit(void)
{
	pthread_mutex_destroy(&sCmdMutex);
}


void CmdRecognize(void)
{
	printf("Recognize readers...\n");
	// If the are threads running, first we have to signal them to kill themselves
	// and the wait them to join before going on.
	if (sThreads.size())
	{
		// Send a user signal to each thread to tell them to die
		vector<pthread_t>::const_iterator threads_citor;
		for (threads_citor = sThreads.begin(); threads_citor != sThreads.end(); ++threads_citor)
			pthread_kill(*threads_citor, SIGUSR1);
		// Wait for threads to join this one
		for (threads_citor = sThreads.begin(); threads_citor != sThreads.end(); ++threads_citor)
			pthread_join(*threads_citor, NULL);
		printf("All threads are killed\n");
		// Close all handles
		vector<hid_device*>::const_iterator const_itor;
		for (const_itor = sHidHandles.begin(); const_itor != sHidHandles.end(); ++const_itor)
			hid_close(*const_itor);
		// Finally reset current thread list
		sThreads.clear();
		// Reset thread counter
		sThreadId = 0;
		// Since after recognition the Readers can be recognized
		// in a different order than before, we reset the TAG table within the DB
		MainConnect.EmptyTable("TAG");
	}
	// Recognize all RFID readers and launch a thread for each of them to do parallel reading
	CHidApi::FindRFIDReadersHids(sHidHandles);
//	printf("HidHandles.Size()==%d\n", HidHandles.size());
	// Loop on them creating one thread per handle
	vector<hid_device*>::const_iterator const_itor;
	for (const_itor = sHidHandles.begin(); const_itor != sHidHandles.end(); ++const_itor)
	{
		pthread_t Thread;
		// Gives current hid_device* as thread cbk parameter
		if (!pthread_create(&Thread, NULL, ThreadCbk, (void*)(*const_itor)))
			sThreads.push_back(Thread);
		else
			printf("ERROR: Fail to create listener Thread!!!");
	}
}

void GetXMLSnapShot(const char **ppXMLSnapShot)
{ 
	static string XMLSnapShot;
	XMLSnapShot = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	XMLSnapShot += "<SnapShot>";
		// Loop on TAG table entries and fill the XML. Use the already open connection to do so.
	if (!MainConnect.SelectFromTable("TAG"))
		return;
	vector<string> Fields;
	while (MainConnect.GetRowStrings(Fields))
//	for(int i=1; i<=2; ++i)
	{
		XMLSnapShot += "<Tag";
		// Write the Reader Id as attribute
		XMLSnapShot += " R=\"";
		XMLSnapShot += Fields[0];
		XMLSnapShot += "\"";
		// Write the tag Id as attribute
		XMLSnapShot += " Id=\"";
		XMLSnapShot += Fields[1];
		XMLSnapShot += "\"";
		// Write the logging time as attribute
		XMLSnapShot += " T=\"";
		XMLSnapShot += Fields[2];
		XMLSnapShot += "\">";
		XMLSnapShot +=  "</Tag>";
	}
	// Add a dummy fps entry, just to test the refresh speed
	static int Count = 1;
	char String[256];
	sprintf(String, "<fps>%d</fps>", Count++);
	XMLSnapShot += String;
	// Close the root
	XMLSnapShot += "</SnapShot>\n";
	*ppXMLSnapShot = XMLSnapShot.c_str();
	//printf(XMLSnapShot);
}

void CommandDispatcher(const char **ppXMLSnapShot, const char Cmd[])
{
	// These mutex could be avoided, since we are using the microwebserver
	// not in multithreaded mode, so each connection is served sequentially by a queue
	// but we leave it here to support the webserver in multithreaded mode
	pthread_mutex_lock(&sCmdMutex);
	// Switch among commands:
	if ( !strcmp(Cmd, "Recog") )
		CmdRecognize();
	// Always returns a DB snapshot
	GetXMLSnapShot(ppXMLSnapShot);	

	pthread_mutex_unlock(&sCmdMutex);
}

