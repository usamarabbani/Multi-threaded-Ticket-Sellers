/*
 * SeatMatrix.h
 *
 *  Created on: Jun 21, 2015
 *  Author: CS 149 Group 6
 */

#ifndef SEATMATRIX_H_
#define SEATMATRIX_H_

#include <string>
#include <pthread.h>

class SeatMatrix {
private:
	std::string seatGrid [10][10];
	int highRow;//these variables make searching slightly faster
	int medRow;
	int lowRow;
	pthread_mutex_t seatMutex;
	int reserveHigh(int* row, int* col);//each type of customer needs different algorithms
	int reserveMed(int* row, int* col);
	int reserveLow(int* row, int* col);
public:
	SeatMatrix();
	int reserveSeat(int* row, int* col, int priceType);
	void giveSeat(int sellerNum, int priceType, int custNum, int row, int col);
	std::string str();
	virtual ~SeatMatrix();
};

#endif /* SEATMATRIX_H_ */
