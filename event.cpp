#include <iostream>
#include "event.h"
#include <cmath>
#include <time.h>
#include <stdlib.h>
using namespace std;

/* Random arrival time generator */
double neg_exp_dis_time(double rate)
{
  double x;
  x = drand48();
  return ((-1 / rate) * log(1 - x));
}

/* Function helper to sort gel list */
bool compare (const Event &event1, const Event &event2)
{
    return ((event1.event_time) < (event2.event_time));
}

/* Random host generator */
int pick_dest(int source, int hosts)
{
  int num = rand() % hosts; //generates a number from (0, (num Hosts-1) )
  if ( num == source ){
    pick_dest(source, hosts);
  }else{
    return num;
  }
  return 0;
}

/* Random Backoff time generator*/
int backoff(int collisions)
{
    int l = pow(2,collisions);
    int x = rand() % l;
    return x; //generates a number from (0, 2^n -1)
}

/* Random frame length generator */
int frame_len_gen()
{
  double x;
  int l;
  x = drand48();
  l = ((-1 / 0.001) * log(1 - x));

  if ( (l > 1544) || (l == 0)){ //saturate l for values above 1544
    frame_len_gen();
  }else{
    return l;
  }
  return l;
}
