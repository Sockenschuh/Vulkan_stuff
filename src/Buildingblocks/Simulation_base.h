#pragma once
#ifndef BASE_H
#define BASE_H

#include <random>
#include <iostream>
#include <cstdint>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float float32;
typedef double float64;

struct compare
{
	int key;
	compare(int const& i) : key(i) {}

	bool operator()(int const& i) {
		return (i == key);
	}
};

int random(int a, int b);

uint32_t randomUint32(uint32_t a, uint32_t b);

int nat_sum(int n);

int cloitreSum(int n);

int diskPixels(int n);

int EigththDiskPixels(int n);

int aproxDiskRangeEigth(int ID);

int circlePixels(int range);



#endif


