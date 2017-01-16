#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

// Number:	Log2:		Sqrt:
// 1		   0		   1
// 2		   1		   1.41421356237
// 4		   2		   2
// 8		   3		   2.82842712475
// 16	           4		   4
// 32		   5		   5.65685424949
// 64		   6		   8
// 128		   7		   11.313708499
// 256		   8		   16
// 512		   9		   22.627416998
// 1024		  10

#define LUT_COUNT 	256
#define LUT_COUNT_M1_D3	85

unsigned char lut[LUT_COUNT];

// log2 source: http://graphics.stanford.edu/~seander/bithacks.html
static unsigned nifty_log2(unsigned v)
{
 static const int MultiplyDeBruijnBitPosition[32] =
 {
   0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
   8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
 };

 v |= v >> 1; // first round down to one less than a power of 2 
 v |= v >> 2;
 v |= v >> 4;
 v |= v >> 8;
 v |= v >> 16;

 return MultiplyDeBruijnBitPosition[(unsigned)(v * 0x07C4ACDDU) >> 27];
}

static unsigned nifty_sqrt(unsigned inv)
{
 unsigned l2 = nifty_log2(inv);
 unsigned index = (((inv * LUT_COUNT_M1_D3) >> (l2 &~1)) - LUT_COUNT_M1_D3);
 unsigned tmp_a = (1U << (l2 >> 1));
 unsigned result = (tmp_a * lut[index] + 127) / 255 + tmp_a;

 return result;
}

int main(int argc, char *argv[])
{
 assert( LUT_COUNT_M1_D3 == ((LUT_COUNT - 1) / 3.0) );

 // Initialize LUT
 for(unsigned i = 0; i < LUT_COUNT; i++)
  lut[i] = 255 * (sqrt(1.0 + (double)i * 3 / LUT_COUNT) - 1.0);

 // Get square root approximation result.
 printf("%u\n", nifty_sqrt(atoi(argv[1])));
}
