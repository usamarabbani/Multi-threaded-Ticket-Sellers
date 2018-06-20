/*
 * SeatMatrix.cpp
 *
 *  Created on: Jun 21, 2015
 *  Author: CS 149 Group 6
 */

#include "SeatMatrix.h"
#include "PriceGuide.h"
#include <sstream>
using namespace std;
SeatMatrix::SeatMatrix()
{
	highRow = 0;
	medRow = 4;
	lowRow = 9;
	pthread_mutex_init(&seatMutex, 0);
	for(int i = 0; i < 10; i++)
	{
		for(int j = 0; j < 10; j++)
		{
			seatGrid[i][j] = "----";
		}
	}

}

/**
 * Reserves a seat
 * @param row a pointer to the caller's row variable
 * @param col a pointer to the caller's col variable
 * @param priceType the price type
 */
int SeatMatrix::reserveSeat(int* row, int* col, int priceType)
{
	pthread_mutex_lock(&seatMutex);
	int success = 0;
	if(priceType == HIGH_PRICE)
	{
		success = reserveHigh(row, col);
	}
	else if(priceType == MED_PRICE)
	{
		success = reserveMed(row, col);
	}
	else if(priceType == LOW_PRICE)
	{
		success = reserveLow(row, col);
	}
	pthread_mutex_unlock(&seatMutex);
	return success;
}

/**
 * Reserves a seat for a high price customer, starting from row 0, col 0 working to the back
 * @param row a pointer to the caller's row variable
 * @param col a pointer to the caller's col variable
 */
int SeatMatrix::reserveHigh(int* row, int* col)
{

	int currentRow = highRow;
	for(int i = currentRow; i < 10; i++)
	{
		for(int j = 0; j < 10; j++)
		{
			if(seatGrid[i][j].compare("----") == 0)
			{
				seatGrid[i][j] = "RESV";
				highRow = i;
				*row = i;
				*col = j;
				return 1;
			}
		}
	}
	return 0;
}

/**
 * Reserves a seat for a medium price customer, alternating around the middle rows
 * @param row a pointer to the caller's row variable
 * @param col a pointer to the caller's col variable
 */
int SeatMatrix::reserveMed(int* row, int* col)
{
	int i = medRow;
	while(i >= 0 && i<= 9)
	{
		if(i <= 4)
		{
			for(int j = 9; j >= 0; j--)
			{
				if(seatGrid[i][j].compare("----") == 0)
				{
					seatGrid[i][j] = "RESV";
					medRow = i;
					*row = i;
					*col = j;
					return 1;
				}
			}
			i = 9 - i;
		}
		else
		{
			for(int j = 0; j < 10; j++)
			{
				if(seatGrid[i][j].compare("----") == 0)
				{
					seatGrid[i][j] = "RESV";
					medRow = i;
					*row = i;
					*col = j;
					return 1;
				}
			}
			i = 8 - i;
		}
	}
	return 0;
}

/**
 * Reserves a seat for a low price customer, starting from row 9, col 9 working to the front
 * @param row a pointer to the caller's row variable
 * @param col a pointer to the caller's col variable
 */
int SeatMatrix::reserveLow(int* row, int* col)
{

	int currentRow = lowRow;
	for(int i = currentRow; i >= 0; i--)
	{
		for(int j = 9; j >= 0; j--)
		{
			if(seatGrid[i][j].compare("----") == 0)
			{
				seatGrid[i][j] = "RESV";
				lowRow = i;
				*row = i;
				*col = j;
				return 1;
			}
		}
	}
	return 0;
}

/**
 * Marks a seat as taken
 * @param sellerNum the number of the seller
 * @param price type the price type of the seller
 * @param custNum the nth customer successfully served by the seller
 * @param row the row to be taken
 * @param col the col to be taken
 */
void SeatMatrix::giveSeat(int sellerNum, int priceType, int custNum, int row, int col)
{
	pthread_mutex_lock(&seatMutex);
	int num = sellerNum * 100 + custNum;
	ostringstream stringStream;
	stringStream << priceChars[priceType];
	stringStream << num;
	seatGrid[row][col] = stringStream.str();
	pthread_mutex_unlock(&seatMutex);
}

/**
 * Returns a string representation of the array
 * @return the array and all that fills it
 */
string SeatMatrix::str()
{
	pthread_mutex_lock(&seatMutex);
	ostringstream stringStream;
	for(int i = 0; i < 10; i++)
	{
		for(int j = 0; j < 10; j++)
		{
			stringStream << '[' << seatGrid[i][j] << "]";
		}
		stringStream << '\n';
	}
	pthread_mutex_unlock(&seatMutex);
	return stringStream.str();
}



SeatMatrix::~SeatMatrix() {
	pthread_mutex_destroy(&seatMutex);
}

