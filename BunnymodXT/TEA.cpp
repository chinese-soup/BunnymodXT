#include "stdafx.hpp"

namespace TEA
{
	// These implementations were taken from Wikipedia, with minor modifications.
	// https://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm
	void encrypt(uint32_t data[2], const uint32_t key[4]) {
		uint32_t v0=data[0], v1=data[1], sum=0, i;           /* set up */
		uint32_t delta=0x9e3779b9;                           /* a key schedule constant */
		uint32_t k0=key[0], k1=key[1], k2=key[2], k3=key[3]; /* cache key */
		for (i=0; i < 32; i++) {                             /* basic cycle start */
			sum += delta;
			v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
			v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
		}                                                    /* end cycle */
		data[0]=v0; data[1]=v1;
	}
//BXTD0}�����J����8��ޮ����J�[�23��hI�%�`
	void decrypt(uint32_t data[2], const uint32_t key[4]) {
		uint32_t v0=data[0], v1=data[1], sum=0xC6EF3720, i;  /* set up */
		uint32_t delta=0x9e3779b9;                           /* a key schedule constant */
		uint32_t k0=key[0], k1=key[1], k2=key[2], k3=key[3]; /* cache key */
		for (i=0; i < 32; i++) {                             /* basic cycle start */
			v1 -= ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
			v0 -= ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
			sum -= delta;
		}                                                    /* end cycle */
		data[0]=v0; data[1]=v1;
	}
}
