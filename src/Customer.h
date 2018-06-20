/*
 * Customer.h
 *
 *  Created on: Jun 22, 2015
 *  Author: CS 149 Group 6
 */

#ifndef CUSTOMER_H_
#define CUSTOMER_H_
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "PriceGuide.h"
//status ints
const int ARRIVING = 0;
const int INLINE = 1;
const int SERVED = 2;
const int OVERWAIT = 3;
const int TURNED_AWAY = 4;
const int SUCCESSFULLY_RESERVED = 5;
//Other constants
const int MAXARRIVAL = 59;
const int OVERTIME = 10;

typedef struct customer
{
	int arrivalTime;
	int procTime;
	int status;
	int sellerNum;
	int index;
	int priceType;
	pthread_mutex_t statusMutex;
	pthread_cond_t arriveCond;//Helps with timing
};

/**
 * @return a randomly generated arrival time
 */
int genArrivalTime()
{
	int rNum = rand();
	return rNum % (MAXARRIVAL + 1);
}

/**
 * @return a randomly generated processing time
 */
int genProcTime(int nPriceType)
{
	int procTime = 0;
	int rNum = rand();
	int max = MAX_PROCT[nPriceType];
	int min = MIN_PROCT[nPriceType];
	procTime = rNum % (max - min + 1) + min;
	return procTime;
}

/**
 * Checks if the customer has waited too long in line. If so, the status is updated.
 * @param cust a pointer to the customer
 * @param currentTime the current time
 * @param startTime the starting time
 */
void updateForOverwait(customer* cust, time_t currentTime,time_t startTime)
{
	int timeElapsed = (int) (currentTime - startTime);
	pthread_mutex_lock(&(cust->statusMutex));
	int status = cust->status;
	if(status == INLINE && timeElapsed > cust->arrivalTime + OVERTIME)
	{
		cust->status = OVERWAIT;
		pthread_cond_signal(&(cust->arriveCond));
	}
	pthread_mutex_unlock(&(cust->statusMutex));
}

/**
 * Retrieves the status of the customer in a thread safe way
 * @param cust the customer pointer
 * @return the status of the customer
 */
int getStatusTS(customer* cust)
{
	int status;
	pthread_mutex_lock(&(cust->statusMutex));
	status = cust-> status;
	pthread_mutex_unlock(&(cust->statusMutex));
	return status;
}

/**
 * Turn away the customer
 * @param cust the customer pointer
 */
void turnAway(customer *cust)
{
	pthread_mutex_lock(&(cust->statusMutex));
	if(cust->status != OVERWAIT && cust->status != SUCCESSFULLY_RESERVED)
	{
		cust->status = TURNED_AWAY;
		pthread_cond_signal(&(cust->arriveCond));
	}
	pthread_mutex_unlock(&(cust->statusMutex));
}

/**
 * Serve customer and change its status if possible
 * @param cust the customer pointer
 * @return true if the customer is servable
 */
bool serve(customer *cust)
{
	bool isServed = false;
	pthread_mutex_lock(&(cust->statusMutex));
	if(cust->status == INLINE)
	{
		cust->status = SERVED;
		isServed = true;
	}
	pthread_mutex_unlock(&(cust->statusMutex));
	return isServed;
}

/**
 * Update the customer status to be successful
 */
void noteSuccess(customer *cust)
{
	pthread_mutex_lock(&(cust->statusMutex));
	cust-> status = SUCCESSFULLY_RESERVED;

	pthread_mutex_unlock(&(cust->statusMutex));
}

/**
 * Returns a boolean determining if the customer is in a final status.
 * @param cust the customer pointer
 * @return true if the customer is turned away, overwaited, or successfully reserved
 */
bool inFinalStatus(customer *cust)
{
	int status;
	pthread_mutex_lock(&(cust->statusMutex));
	status = cust-> status;
	pthread_mutex_unlock(&(cust->statusMutex));
	return status == TURNED_AWAY || status == OVERWAIT || status == SUCCESSFULLY_RESERVED;
}



/**
 * Blocks the thread until the customer has entered the queue
 * @param cust the customer pointer
 */
void waitForArrival(customer * cust)
{
	pthread_mutex_lock(&(cust->statusMutex));
	while(cust->status == ARRIVING)
	{
		pthread_cond_wait(&(cust->arriveCond), &(cust->statusMutex));
	}
	pthread_mutex_unlock(&(cust->statusMutex));
}

#endif /* CUSTOMER_H_ */
