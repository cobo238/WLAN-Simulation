#ifndef EVENT_H
#define EVENT_H

#include <queue>
using namespace std;

class Event
{
  public:
    double event_time;
    int event_type;
    /* Types of Events:
    1 for arrival
    2 for receiving
    3 for difs timer
    4 for sifs timer
    5 for ack
    6 for ack timeout
    */
    double transmission_time;
    int frame_length;
    int source_host;
    int dest_host;
    double timeout;
    double enqueue_time;
    Event(double time , double trans, int type, int data_len,
          int s_host, int d_host, double t_out, double in_q)
    {
      event_time = time;
      transmission_time = trans;
      event_type = type;
      frame_length = data_len;
      source_host = s_host;
      dest_host = d_host;
      timeout = t_out;
      enqueue_time = in_q;
    }
};

class Host
{
  public:
    queue<Event> buffer;
    int length;
};

double neg_exp_dis_time(double rate);
bool compare (const Event &event1, const Event &event2);
int pick_dest(int source, int hosts);
int backoff(int collisions);
int frame_len_gen();

#endif
