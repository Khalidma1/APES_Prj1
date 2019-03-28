/*
*		File: main.c
*		Purpose: main file of the APES Project 1
*		Owners: Poorn Mehta & Khalid AlAwadhi
*		Last Modified: 3/28/2019
*/

#include "main.h"

/* Our includes */
#include "Global_Defines.h"
#include "My_Time.h"
#include "POSIX_Qs.h"
#include "LoggingThread.h"
#include "SocketThread.h"
#include "TempThread.h"
#include "LuxThread.h"
#include "My_Time.c"
#include "POSIX_Qs.c"
#include "LoggingThread.c"
#include "SocketThread.c"
#include "TempThread.c"
#include "LuxThread.c"


/* Global Variables */
pthread_t Log_pThread, Socket_pThread, Temp_pThread, Lux_pThread;

pthread_mutex_t lock, lock_var;					//Used to lock and unlock critical sections in code
sig_atomic_t flag = 0;			//This will indicate whether any valid user signal has been received or not
uint8_t sig_sync = 0;					//Used to toggle in signal_function()
uint8_t LogKillSafe = 3;		//Used when other threads are killed. They decrement this value which assures Logging pThread is killed last.
uint8_t AliveThreads = 0x00;	//Used in Main and checked bitwise to see which pThreads are alive

uint8_t Counter = 0;

int temp_file_des = -1;
int lux_file_des = -1;
uint8_t Temp_Error_Retry = 0;
uint8_t Lux_Error_Retry = 0;
uint8_t Temp_Sensor_State = Sensor_Offline;
uint8_t Lux_Sensor_State = Sensor_Offline;

uint8_t Socket_State = Socket_Offline;


/*
 * LAST WORKING ON:
 */

/*########################################################################################
 #                                    TO-DO:                                             #
 #########################################################################################
 *****************************************************************************************
 * GENERAL                                                                               *
 *****************************************************************************************
 * 1- [COMPLETED] REPLACE ALL PERRORS WITH strerror_r() TO BE ABLE TO LOG IT VIA THE
 * 				| LOGGING PTHREAD
 * 				L--> CREATED Log_error() FOUND IN POSIX_Qs.h
 *
 * 2- [COMPLETED] UPDATE SendToThreadQ() TO USE PARAMETERS INSTEAD OF STRUCT INPUT
 *
 * 3- [COMPLETED] FIX THREAD KILLING BUG
 *
 * 4- []
 *
 *****************************************************************************************
 * MAIN THREAD                                                                           *
 *****************************************************************************************
 * 1- [COMPLETED] FIX BUG OF GETTING USER LOG FILE PATH
 *
 * 2- [COMPLETED] IMPLEMENT METHOD TO CHECK CHILD THREADS ARE ALIVE AT SOME INTERVAL
 * 				L--> USED AliveThreads GLOBAL VARIABLE (CODE IN MAIN)
 *
 * 3- [COMPLETED] IMPLEMENT METHOD TO CLEANLY EXIT WHEN REQUESTED (MAKE CHILD EXITS PROPERLY
 * 		 THEN MAIN)
 *
 * 4- [] LOG ERROR INFORMATION AND INDICATE ERROR WITH BB USR LEDS (EG. MISSING SENSOR)
 *
 * 5- [COMPLETED] CREATE MY OWN TIME GET FUNCTION
 * 				L--> FOUND IN My_Time .h/.c
 *
 * 6- [COMPLETED] IMPLEMENT SIGNAL HANDLER
 *
 * 7- [COMPLETED] DISPLAY THREAD IDS AT START-UP
 *
 * 8- [SCRAPPED] KILL ONLY SPECIFIED THREADS
 *
 * 9- []
 *
 *
 *****************************************************************************************
 * LOGGING THREAD                                                                        *
 *****************************************************************************************
 * 1- [COMPLETED] CREATE LOGGING PROTOTYPE
 * 				L--> CREATED LoggingThread.h/.c
 *
 * 2- [COMPLETED] CREATE MSG STRUCTURE
 * 				L--> FOUND IN OurDefines.h
 *
 * 3- [COMPLETED] TEST LOGGING
 *
 * 4- [COMPLETED] CREATE LOGGING PTHREAD
 *
 * 5- [COMPLETED] CREATE POSIX QUEUE FOR LOGGING THREAD
 *
 * 6- [COMPLETED] TEST SENDING MSG FROM pMAIN -> pLOGGING
 *
 * 7- [COMPLETED] ADD FILE PATH ERROR HANDLING
 *
 * 8- [COMPLETED] LOG FILE PATH CONFIGURABLE AT RUN-TIME
 *
 * 9- [COMPLETED] START-UP LOGGING SUCCESS/FAIL REPORT
 *
 * 10- [COMPLETED] ADD LOG LEVEL TO STRUCT
 * 				L--> ADDED NEW ELEMENT TO OUR MSG STRUCT (STRING)
 *
 * 11- [COMPLETED] CREATE A MSG SEND TO QUEUE FUNCTION
 * 				L--> CREATED POSIX_Qs .h/.c
 *
 * 12- [COMPLETED] MAKE LOGGING THREAD BLOCK AND WAIT FOR ANY MESSAGES
 *
 * 13- [COMPLETED] HANDLE INITS OF OTHER THREAD TO LOGGING THREAD
 *
 * 14- []
 *
 *
 *****************************************************************************************
 * SOCKET THREAD                                                                         *
 *****************************************************************************************
 * 1- [POORN HANDLED IT] CREATE THE INTERNAL STRUCTURE OF THE SOCKET QUEUE
 *
 * 2- [COMPLETED] ADD POORN CODE
 *
 * 3- []
 *
 *
 *****************************************************************************************
 * TEMP THREAD                                                                           *
 *****************************************************************************************
 * 1- [COMPLETED] INIT THIS THREAD
 *
 * 2- [COMPLETED] ADD POORN CODE
 *
 * 3- []
 *
 *
 *****************************************************************************************
 * LUX THREAD                                                                            *
 *****************************************************************************************
 * 1- [COMPLETED] INIT THIS THREAD
 *
 * 2- [COMPLETED] ADD POORN CODE
 *
 * 3- []
 *
 *
#########################################################################################*/


void signal_function(int value)
{
	if(value == SIGVTALRM)
	{
			if(Socket_State == Socket_Online)
			{
//				pthread_mutex_lock(&lock_var);
				AliveThreads |= SOCKET_ALIVE;
//				pthread_mutex_unlock(&lock_var);
			}
			Counter += 1;
			if(Counter == Counter_Threshold)
			{
					Counter = 0;
					if(Temp_Error_Retry > Temp_No_Retry)
					{
							Temp_Error_Retry -= 1;
							if(close(temp_file_des))		Log_error(Main,"Closing the Temperature I2C File", errno, LOGGING_AND_LOCAL);
							SendToThreadQ(Main, Logging, "INFO", "\nTrying to get the Temperature Sensor Online... Calling TempThread_Init()\n");
							if(TempThread_Init())		Log_error(Main,"Attempt to get the Temperature Sensor Online Failed... Exiting TempThread_Init()", errno, LOGGING_AND_LOCAL);
							else
							{
								Temp_Error_Retry = Temp_No_Retry;
								Temp_Sensor_State = Sensor_Online;
								SendToThreadQ(Main, Logging, "INFO", "\nTemperature Sensor is Now Online...\n");
							}
					}

					if(Lux_Error_Retry > Lux_No_Retry)
					{
							Lux_Error_Retry -= 1;
							if(close(lux_file_des))		Log_error(Main,"Closing the Lux I2C File", errno, LOGGING_AND_LOCAL);
							SendToThreadQ(Main, Logging, "INFO", "\nTrying to get the Lux Sensor Online... Calling LuxThread_Init()\n");
							if(LuxThread_Init())		Log_error(Main,"Attempt to get the Lux Sensor Online Failed... Exiting LuxThread_Init()", errno, LOGGING_AND_LOCAL);
							else
							{
								Lux_Error_Retry = Lux_No_Retry;
								Lux_Sensor_State = Sensor_Online;
								SendToThreadQ(Main, Logging, "INFO", "\nLux Sensor is Now Online...\n");
							}
					}
			}
			if(sig_sync == 0)
			{
				flag = Temperature_Signal;
				sig_sync = 1;
			}
			else
			{
				flag = Lux_Signal;
				sig_sync = 0;
			}
	}
	else
	{
			flag = value;
			Socket_State = Socket_Offline;
			if(kill_socket_init())        printf("\nSocket killing failed\n");
	}
}

void sig_setup(void)
{
		// Configuring timer and signal action
		struct sigaction custom_signal_action;
		struct itimerval custom_timer;

		// Set all initial values to 0 in the structure
		memset(&custom_signal_action, 0, sizeof (custom_signal_action));

		// Set signal action handler to point to the address of the target function (to execute on receiving signal)
		custom_signal_action.sa_handler = &signal_function;

		// Setting interval according to define in main.h
		custom_timer.it_interval.tv_sec = 0;
		custom_timer.it_interval.tv_usec = Timer_Interval * 1000;

		// Setting initial delay to 2s
		custom_timer.it_value.tv_sec = 2;
		custom_timer.it_value.tv_usec = 0;

		// Setting the signal action to kick in the handler function for these 3 signals
		sigaction(SIGVTALRM, &custom_signal_action, 0);
		sigaction(SIGUSR1, &custom_signal_action, 0);
		sigaction(SIGUSR2, &custom_signal_action, 0);

		// Starting timer
		setitimer(ITIMER_VIRTUAL, &custom_timer, 0);
}




int main(int argc, char *argv[])
{
	struct Pthread_ArgsStruct args;						//Create the pthread args structure

	char User_LogFilePath[100];							//This will store the log file path location to pass to the Logging pthread

	printf("Starting... PID: %d\n\n", getpid());

	/* Check if the user entered a logfile path */
	if(argc > 1)
	{
		sprintf(User_LogFilePath, "%s", argv[1]);
		printf("Chosen log file path: %s\n", User_LogFilePath);
	}

	/* Else, use default logfile path */
	else
	{
		sprintf(User_LogFilePath, "./LogFile.txt");
		printf("No logfile path chosen. Using default location './LogFile.txt'\n\n");
	}


	/* Store filepath to pass to pThreads */
	strcpy(args.LogFile_Path, User_LogFilePath);


	if( pthread_mutex_init(&lock, NULL) != 0 )
	{
		Log_error(Main, "pthread_mutex_init()", errno, LOCAL_ONLY);
		return -1;
	}

	if( pthread_mutex_init(&lock_var, NULL) != 0 )
	{
		Log_error(Main, "pthread_mutex_init()", errno, LOCAL_ONLY);
		return -1;
	}


	/* Create the needed pThreads */
	pthread_t Log_pThread, Socket_pThread, Temp_pThread, Lux_pThread;


	/* Create Logging pThread */
	if(pthread_create(&Log_pThread, NULL, &LoggingThread, (void *)&args) != 0)
	{
		Log_error(Main, "Logging pthread_create()", errno, LOCAL_ONLY);
	}
	else
	{
		printf("[%lf] Main pThread SUCCESS: Created Logging Thread!\n\n", GetCurrentTime());
	}

	/* Need to sleep a bit to make sure the Logging Thread starts up first */
	sleep(2);



	//BIST SHOULD BE HERE



	/* Create Socket pThread */
	if(pthread_create(&Socket_pThread, NULL, &SocketThread, NULL) != 0)
	{
		Log_error(Main, "Socket pthread_create()", errno, LOGGING_AND_LOCAL);
	}
	else
	{
		printf("[%lf] Main pThread SUCCESS: Created Socket Thread!\n\n", GetCurrentTime());
	}
		sleep(2);

	/* Create Temp pThread */
	if(pthread_create(&Temp_pThread, NULL, &TempThread, NULL) != 0)
	{
		Log_error(Main, "Temp pthread_create()", errno, LOGGING_AND_LOCAL);
	}
	else
	{
		printf("[%lf] Main pThread SUCCESS: Created Temp Thread!\n\n", GetCurrentTime());
	}
		sleep(2);


	/* Create Lux pThread */
	if(pthread_create(&Lux_pThread, NULL, &LuxThread, NULL) != 0)
	{
		Log_error(Main, "Lux pthread_create()", errno, LOGGING_AND_LOCAL);
	}
	else
	{
		printf("[%lf] Main pThread SUCCESS: Created Lux Thread!\n\n", GetCurrentTime());
	}

	/* Let other pThreads execute before checking */
	sleep(2);

	/* Setting up Signals */
	sig_setup();

	/* While there is at least one thread alive: */
	while( AliveThreads != 0 )
	{
		/* Create a copy of the global variable, AliveThreads, and store it in the local variable CurrentAlive.
		 * This is done as to not halt the other pThreads trying to use AliveThreads. It is faster to update
		 * local variable and do work rather than halt all threads until the alive check is done. */
		pthread_mutex_lock(&lock_var);
		uint8_t CurrentAlive = AliveThreads;		//Create a copy of the global variable
		AliveThreads = 0;							//Reset alive bits
		pthread_mutex_unlock(&lock_var);


		/* Check Logging pThread */
		if(CurrentAlive & LOGGING_ALIVE)
		{
		//	SendToThreadQ(Main, Logging, "INFO", "Logging pThread is alive");
			printf("[%lf] Main pThread(INFO): Logging pThread is alive\n\n", GetCurrentTime());
		}
		else
		{
			Log_error(Main, "Logging pThread is not alive", 42, LOCAL_ONLY);
		}

		/* Check Socket pThread */
		if(CurrentAlive & SOCKET_ALIVE)
		{
			printf("[%lf] Main pThread(INFO): Socket pThread is alive\n\n", GetCurrentTime());
		}
		else
		{
			Log_error(Main, "Socket pThread is not alive", 42, LOCAL_ONLY);
		}

		/* Check Temp pThread */
		if(CurrentAlive & TEMP_ALIVE)
		{
			printf("[%lf] Main pThread(INFO): Temp pThread is alive\n\n", GetCurrentTime());
		}
		else
		{
			Log_error(Main, "Temp pThread is not alive", 42, LOCAL_ONLY);
		}

		/* Check Lux pThread */
		if(CurrentAlive & LUX_ALIVE)
		{
			printf("[%lf] Main pThread(INFO): Lux pThread is alive\n\n", GetCurrentTime());
		}
		else
		{
			Log_error(Main, "Lux pThread is not alive", 42, LOCAL_ONLY);
		}

		/* Check again after 10 secs */
		sleep(Alive_Testing_Interval);
	}

	printf("[%lf] Main pThread(INFO): All Threads were terminated. Exiting...\n\n", GetCurrentTime());

	/* Wait for pThreads to finish */
	pthread_join(Log_pThread, NULL);
	pthread_join(Socket_pThread, NULL);
	pthread_join(Temp_pThread, NULL);
	pthread_join(Lux_pThread, NULL);
}
