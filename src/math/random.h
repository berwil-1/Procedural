#pragma once

inline int random()
{
	static int x =
		362436069;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return x;
}

inline uint urandom()
{
	static uint x =
		362436069;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return x;
}