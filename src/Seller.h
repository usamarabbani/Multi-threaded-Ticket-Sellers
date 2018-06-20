/*
 * Seller.h
 *
 *  Created on: Jun 22, 2015
 *  Author: CS 149 Group 6
 */

#ifndef SELLER_H_
#define SELLER_H_

#include "Customer.h"
typedef struct seller
{
	int sellerNum;
	int priceType;
	int customerServed;
	int startIndex;
	customer* custArr;
};


/**
 * Attempt to serve a customer
 * @param s the seller pointer
 * @param custArrSize the size of the customer array
 * @return the index of the servable customer, or -1 if there is none
 */
int tryServe(seller *s, int custArrSize)
{
	int startIndex = s->startIndex;
	for(int i = startIndex; i < custArrSize; i++)
	{
		waitForArrival(&(s->custArr[i]));
		if(serve(&(s->custArr[i])))
		{
			//s->startIndex = i + 1;
			return i;
		}
		else if(inFinalStatus(&(s->custArr[i])))
		{
			s->startIndex = i + 1;
		}
	}
	return -1;
}

/**
 * Checks if there are no more servable customers
 * @param s the seller pointer
 * @param custArrSize
 * return true if startIndex is beyond the array bounds
 */
bool isDone(seller *s, int custArrSize)
{
	return s->startIndex >= custArrSize;
}



#endif /* SELLER_H_ */
