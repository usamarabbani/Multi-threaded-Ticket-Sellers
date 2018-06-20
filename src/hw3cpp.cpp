//============================================================================
// Name        : hw3cpp.cpp
// Author      : CS 149 Group 6
// Version     : 6/24/15
// Copyright   : Your copyright notice
// Description : Uses pthreads to simulate ticket sellers
//============================================================================

#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <sstream>
//Including one header means including all the headers that the other header included
#include "Customer.h"
#include "Seller.h"
#include "PriceGuide.h"
#include "SeatMatrix.h"
using namespace std;
//Global vars
seller sellers[10];
int CUST_PER_SELLER = 5; //Default value in case user botches the argument
int NUM_HIGH_SELLERS = 1;
int NUM_MED_SELLERS = 3;
int NUM_LOW_SELLERS = 6;
time_t timeLimit = 60;
SeatMatrix seats;
vector<string> LOG;//To help with file output. I want to print each event 1 at a time
time_t startTime;
pthread_mutex_t timeMutex; //I have no idea if time(NULL) is thread safe
pthread_mutex_t logMutex; //Same thing with vector.push_back(...)

/**
 * A threadsafe wrapper for time(NULL)
 * @return the current time since the epoch
 */
time_t tsTime()
{
	time_t result;
	pthread_mutex_lock(&timeMutex);
	result = time(NULL);
	pthread_mutex_unlock(&timeMutex);
	return result;
}
/**
 * Formats the given time value into H:MM
 * @param minutes the number of minutes elapsed, which should be < 68
 * @return the formatted string representation of the elapsed minutes
 */
string formatMinute(time_t minutes)
{
	int hour = minutes / 60;
	int remain = minutes % 60;
	ostringstream stream;
	stream << hour << ':';
	if(remain < 10)
	{
		stream << '0';
	}
	stream << remain;
	return stream.str();
}

/**
 * Formats the event message, print it to screen, and save it to log
 * @param priceType the type of price
 * @param sellerNum the number of the seller
 * @param message the message describing the event
 */
void formatEvent(int priceType, int sellerNum, int custIndex, string message)
{
	ostringstream stream;
	string formattedTime = formatMinute(tsTime() - startTime);
	stream << formattedTime << ": Seller " << priceChars[priceType] <<  sellerNum << " / Customer ";
	if(custIndex < 10){
		stream << '0';
	}
	stream << custIndex << " - " << message;
	pthread_mutex_lock(&logMutex);
	LOG.push_back(stream.str());
	cout<< stream.str() << endl;
	pthread_mutex_unlock(&logMutex);
}

/**
 * A sorting function to help sort customers by arrival
 * @param &a the first customer passed by reference
 * @param &b the second customer passed by reference
 * return true if a arrives first, false otherwise
 */
bool sortByArrival(customer const &a, customer const &b) {
	return a.arrivalTime < b.arrivalTime;
}

/**
 * @return true if the time limit is reached
 */
bool timedOut(){return tsTime() - startTime >= timeLimit;}

/**
 * Puts the customer in line
 * This needs to be in the main file because it needs immediate access to the log and cout
 * @param cust the customer pointer
 */
void getInline(customer * cust)
{
	pthread_mutex_lock(&(cust->statusMutex));
	cust-> status = INLINE;
	formatEvent(cust->priceType, cust->sellerNum, cust -> index, "Arrived in queue");
	pthread_cond_signal(&(cust->arriveCond));
	//I probably don't need to signal anywhere else but here, but signaling is a no-op at worst anyways
	pthread_mutex_unlock(&(cust->statusMutex));
}

/**
 * Turn away all customers
 * @param s the seller pointer
 * @param custArrSize
 */
void turnAwayAll(seller* s, int custArrSize)
{

	for(int i = 0; i < custArrSize; i++)
		{
			//int status = getStatusTS(&(s->custArr[i]));
			if(!inFinalStatus(&(s->custArr[i])))
			{
				turnAway(&(s->custArr[i]));
				formatEvent(s->priceType, s->sellerNum, i + 1, "Leaving due to seats being sold out.");
			}
		}

}

/**
 * A function pointer for the customer threads
 * @param param a pointer to the customer, though casting will be needed
 */
void* custThrFun(void* param) {
	customer* custP = (customer*) param;
	ostringstream indStream;
	indStream << custP->index;
	sleep(custP->arrivalTime);
	getInline(custP);

	while(!timedOut())
	{
		updateForOverwait(custP, tsTime(), startTime);
		int status = getStatusTS(custP);
		if(status == OVERWAIT)
		{
			formatEvent(custP->priceType, custP->sellerNum, custP->index, "Waited too long in line, leaving now");
			pthread_exit(NULL);
		}
		else if(status == TURNED_AWAY)
		{

			pthread_exit(NULL);
		}
		else if(status == SUCCESSFULLY_RESERVED)
		{
			pthread_exit(NULL);
		}
		sleep(1);
	}

	pthread_mutex_lock(&(custP->statusMutex));
	int finalStatus = custP->status;
	if(finalStatus != SERVED && finalStatus != OVERWAIT && finalStatus != SUCCESSFULLY_RESERVED && finalStatus != TURNED_AWAY)
	{
		custP->status= TURNED_AWAY;
		formatEvent(custP->priceType, custP->sellerNum, custP->index, "Ticket selling time is over, leaving now.");
	}
	pthread_cond_signal(&(custP->arriveCond));
	pthread_mutex_unlock(&(custP->statusMutex));
	pthread_exit(NULL);
}

/**
 * A function pointer for the seller threads
 * @param param a pointer to the customer
 */
void* sellThrFun(void* param) {
	seller* sellP = (seller*) param;
	while(!timedOut() && !isDone(sellP, CUST_PER_SELLER))
	{
		int row;
		int col;
		int procTime;
		int index = tryServe(sellP, CUST_PER_SELLER);
		if(index != -1)
		{
			procTime = sellP->custArr[index].procTime;
			int status = seats.reserveSeat(&row, &col, sellP->priceType);
			if(status == 0)
			{
				turnAwayAll(sellP, CUST_PER_SELLER);
				pthread_exit(NULL);
			}
			noteSuccess(&(sellP->custArr[index]));
			formatEvent(sellP->priceType, sellP->sellerNum, index + 1 ,"Obtaining seat reservation");
			sleep(procTime);
			seats.giveSeat(sellP->sellerNum, sellP->priceType, index + 1, row, col);
			sellP->customerServed = sellP->customerServed + 1;
			string formattedTime = formatMinute(tsTime() - startTime);
			ostringstream soldSeatStr;
			soldSeatStr << "Bought and is going to seat row " << (row + 1) << " col " << (col + 1) << '\n';
			string result = soldSeatStr.str() + seats.str();
			formatEvent(sellP->priceType, sellP->sellerNum, index + 1, result);
		}
		sleep(1);
	}
	pthread_exit(NULL);
}

int main(int argc, const char * argv[]) {

	if (argc == 2) {
		CUST_PER_SELLER = (int) strtol(argv[1], NULL, 10);
	}
	srand(time(NULL));
	int sellerNum = 1;
	for (int i = 0; i < NUM_HIGH_SELLERS; i++) {
		sellers[i].sellerNum = sellerNum;
		sellers[i].priceType = HIGH_PRICE;
		sellers[i].customerServed = 0;
		sellers[i].startIndex = 0;
		sellers[i].custArr = new customer[CUST_PER_SELLER];

		for (int j = 0; j < CUST_PER_SELLER; j++) {
			sellers[i].custArr[j].arrivalTime = genArrivalTime();
			sellers[i].custArr[j].status = ARRIVING;
			sellers[i].custArr[j].priceType = HIGH_PRICE;
			sellers[i].custArr[j].procTime = genProcTime(HIGH_PRICE);
			sellers[i].custArr[j].sellerNum = sellerNum;
			pthread_mutex_init(&(sellers[i].custArr[j].statusMutex), 0);
			pthread_cond_init(&(sellers[i].custArr[j].arriveCond), 0);
		}
		//this will emulate a FCFS situation
		sort(sellers[i].custArr, sellers[i].custArr + CUST_PER_SELLER,
				sortByArrival);
		//indexing after sorting makes things easier for the seller thread.
		for (int j = 0; j < CUST_PER_SELLER; j++)
		{
			sellers[i].custArr[j].index = j + 1;
		}
		sellerNum++;
	}

	sellerNum = 1;
	for (int i = NUM_HIGH_SELLERS; i < NUM_HIGH_SELLERS + NUM_MED_SELLERS;
			i++) {
		sellers[i].sellerNum = sellerNum;
		sellers[i].priceType = MED_PRICE;
		sellers[i].customerServed = 0;
		sellers[i].startIndex = 0;
		sellers[i].custArr = new customer[CUST_PER_SELLER];
		for (int j = 0; j < CUST_PER_SELLER; j++) {
			sellers[i].custArr[j].arrivalTime = genArrivalTime();
			sellers[i].custArr[j].status = ARRIVING;
			sellers[i].custArr[j].priceType = MED_PRICE;
			sellers[i].custArr[j].procTime = genProcTime(MED_PRICE);
			sellers[i].custArr[j].sellerNum = sellerNum;
			pthread_mutex_init(&(sellers[i].custArr[j].statusMutex), 0);
			pthread_cond_init(&(sellers[i].custArr[j].arriveCond), 0);
		}
		sort(sellers[i].custArr, sellers[i].custArr + CUST_PER_SELLER,
				sortByArrival);
		for (int j = 0; j < CUST_PER_SELLER; j++)
		{
			sellers[i].custArr[j].index = j + 1;
		}
		sellerNum++;
	}

	sellerNum = 1;
	for (int i = NUM_HIGH_SELLERS + NUM_MED_SELLERS; i < 10; i++) {
		sellers[i].sellerNum = sellerNum;
		sellers[i].priceType = LOW_PRICE;
		sellers[i].customerServed = 0;
		sellers[i].startIndex = 0;
		sellers[i].custArr = new customer[CUST_PER_SELLER];
		for (int j = 0; j < CUST_PER_SELLER; j++) {
			sellers[i].custArr[j].arrivalTime = genArrivalTime();
			sellers[i].custArr[j].status = ARRIVING;
			sellers[i].custArr[j].priceType = LOW_PRICE;
			sellers[i].custArr[j].procTime = genProcTime(LOW_PRICE);
			sellers[i].custArr[j].sellerNum = sellerNum;
			pthread_mutex_init(&(sellers[i].custArr[j].statusMutex), 0);
			pthread_cond_init(&(sellers[i].custArr[j].arriveCond), 0);
		}

		sort(sellers[i].custArr, sellers[i].custArr + CUST_PER_SELLER,
				sortByArrival);
		for (int j = 0; j < CUST_PER_SELLER; j++)
		{
			sellers[i].custArr[j].index = j + 1;
		}
		sellerNum++;
	}

	//create threads for each seller and customer
	pthread_mutex_init(&timeMutex, 0);
	pthread_mutex_init(&logMutex, 0);
	startTime = time(NULL);
	vector<pthread_t> threads;//this is to help join threads, since the threads are really numerical identifiers
	for(int i = 0; i < 10; i++)
	{
		for(int j = 0; j < CUST_PER_SELLER; j++)
		{
			pthread_t custThr;
			pthread_create(&custThr, NULL, custThrFun, (void*) &(sellers[i].custArr[j]));
			threads.push_back(custThr);
		}
		pthread_t sellThr;
		pthread_create(&sellThr, NULL, sellThrFun, (void*) &(sellers[i]));
		threads.push_back(sellThr);
	}

	//join threads
	for(unsigned i = 0; i < threads.size(); i++)
	{
		pthread_t thr = threads[i];
		pthread_join(thr, NULL);
	}
	pthread_mutex_destroy(&timeMutex);
	pthread_mutex_destroy(&logMutex);
	//each array index corresponds to the int representation of a pricetype
	int successArr[] = {0, 0, 0};
	int turnawayArr[] = {0, 0, 0};
	int overwaitArr[] = {0, 0, 0};
	for(int i = 0; i < 10; i++)
	{
		for(int j = 0; j < CUST_PER_SELLER; j++)
		{
			pthread_mutex_destroy(&(sellers[i].custArr[j].statusMutex));
			pthread_cond_destroy(&(sellers[i].custArr[j].arriveCond));
			int status = sellers[i].custArr[j].status;
			int priceType = sellers[i].priceType;
			if(status == SUCCESSFULLY_RESERVED)
			{
				successArr[priceType]++;
			}
			else if(status == TURNED_AWAY)
			{
				turnawayArr[priceType]++;
			}
			else if(status == OVERWAIT)
			{
				overwaitArr[priceType]++;
			}
		}
	}

	ostringstream lastStats;
	lastStats << "\n\n";
	for(int i = 0; i <= LOW_PRICE; i++)
	{
		lastStats << priceChars[i] << " seated: " << successArr[i] << '\n';
		lastStats << priceChars[i] << " turned away: " << turnawayArr[i] << '\n';
		lastStats << priceChars[i] << " overwaited: " << overwaitArr[i] << '\n';
	}
	cout << lastStats.str() << endl;
	LOG.push_back(lastStats.str());

	//Convert output to file
	ostringstream fileName;
	fileName << "output" << CUST_PER_SELLER << ".txt";
	ofstream outFile;
	outFile.open(fileName.str().c_str()); //I have to convert it to string, then to c string
	for(unsigned i = 0; i < LOG.size(); i++)
	{
		outFile << LOG[i] << '\n';
	}
	outFile.close();
	cout << "The output is likely too large for the terminal, so look for " << fileName.str() << endl;
	exit(0);
}
