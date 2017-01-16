// g++ -Wall -O2 -o alfalfas alfalfas.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef unsigned int uint32;
typedef unsigned long long uint64;

struct RNG
{
 void ResetState(void)  // Must always reset to the same state.
 {
  x = 123456789;
  y = 987654321;
  z = 43219876;
  c = 6543217;
 }

 uint32 x,y,z,c;

 uint32 RandU32(void)
 {
  uint64 t;

  x = 314527869 * x + 1234567;
  y ^= y << 5; y ^= y >> 7; y ^= y << 22;
  t = 4294584393ULL * z + c; c = t >> 32; z = t;

  return (x + y + z);
 }

 uint32 RandU32(uint32 mina, uint32 maxa)
 {
  const uint32 range_m1 = maxa - mina;
  uint32 range_mask;
  uint32 tmp;

  range_mask = range_m1;
  range_mask |= range_mask >> 1;
  range_mask |= range_mask >> 2;
  range_mask |= range_mask >> 4;
  range_mask |= range_mask >> 8;
  range_mask |= range_mask >> 16;

  do
  {
   tmp = RandU32() & range_mask;
  } while(tmp > range_m1);

  return(mina + tmp);
 }
};

#define WIDTH 100
#define HEIGHT 100
static char grid[WIDTH][HEIGHT];

static const int pmatch_min = 33;
static const char* theword = "ALFALFAS";

int main()
{
 RNG rng;
 //char possibles[8] = { 'A', 'L', 'F', 'A', 'L', 'F', 'A', 'S' };

 rng.ResetState();

for(;;)
{
 //puts("ITER");
 unsigned matchcount = 0;
 unsigned pmatchcount = 0;
 int solvex, solvey;

 for(unsigned x = 0; x < WIDTH; x++)
 {
  for(unsigned y = 0; y < HEIGHT; y++)
  {
   grid[x][y] = theword[rng.RandU32(0, strlen(theword) - 1)];
  }
 }

 for(int x = 0; x < WIDTH; x++)
 {
  for(int y = 0; y < HEIGHT; y++)
  {
   int xs[8] = { 1, 0, -1,  0, 1, -1, 1, -1 };
   int ys[8] = { 0, 1,  0, -1, 1,  1, -1, -1 };

   for(int sd = 0; sd < 8; sd++)
   {
    int subx = x;
    int suby = y;
    static const char* ss = theword;
    static const int ss_len = strlen(theword);
    int z;

    for(z = 0; z < ss_len; z++, subx += xs[sd], suby += ys[sd])
    {
     if(subx >= 0 && subx < WIDTH && suby >= 0 && suby < HEIGHT)
     {
      if(grid[subx][suby] != ss[z])
       break;
     }
     else
      break;
    }
    if(z == (ss_len - 1))
    {
     pmatchcount++;
    }

    if(z == ss_len)
    {
     solvex = x;
     solvey = y;
     matchcount++;
    }
   }

  }
 }

 if(matchcount == 1 && pmatchcount >= pmatch_min)
 {
  printf("Size: %d x %d\n", WIDTH, HEIGHT);
  printf("Solution: %d %d (counting from zero)\n", solvex, solvey);
  printf("pmatchcount: %d\n", pmatchcount);
  printf("Word to find: %s\n\n", theword);
  for(int y = 0; y < WIDTH; y++)
  {
   for(int x = 0; x < HEIGHT; x++)
   {
    //if(rng.RandU32(0, 1) == 0)
     printf("%c", grid[x][y]);
    //else
    // printf(" %c", grid[x][y]);
   }
   printf("\n");
  }
  break;
 }

}

}
