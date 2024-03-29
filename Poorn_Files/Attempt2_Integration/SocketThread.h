/*
 * SocketThread.h
 *
 *      Author: Khalid AlAwadhi
 *      Spring 2019
 */

#ifndef SOCKETTHREAD_H_
#define SOCKETTHREAD_H_

#include "main.h"

/**************************************************************************************************************
 * USAGE: This function contains all what the socket pThread will do.
 *
 * PARAMETERS:
 *            - NONE
 *
 * RETURNS: NONE
 *
 **************************************************************************************************************/
void * SocketThread(void * args);



/**************************************************************************************************************
 * USAGE: This function will be called initially when the Socket Thread is first initialized.
 *
 * PARAMETERS:
 *            - NONE
 *
 * RETURNS: NONE
 *
 **************************************************************************************************************/
void SocketThread_Init();


#endif /* SOCKETTHREAD_H_ */
