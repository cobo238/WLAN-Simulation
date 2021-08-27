#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <list>
#include <string>
#include "event.h"
using namespace std;

#define DIFS 0.1
#define SIFS 0.05
#define ACK_TRANS 0.0465

int busy;   //0 for idle and 1 for busy
double channel_cap = 11000000;
int collisions;
int total_bytes;
int packets;
double trans_delay;
double queue_delay;

list<Event> gel; //create global event list


void start_difs(double t, double trans_t, int src, int des, int data_len, double in_q)
{
  if (busy == 0)  //if channel is idle
  {
    //create difs timer event
    Event difs_timer(t + DIFS, trans_t, 3, data_len, src, des, 0, in_q);
    trans_delay = trans_delay + DIFS; //add the difs to trans delay
    gel.push_back(difs_timer);  //insert difs timer event in gel
    gel.sort(compare);
  }else{  //else channel is busy
    collisions++;
    int back_off = backoff(collisions);
    //create difs timer event
    Event difs_timer(t + DIFS + back_off, trans_t, 3, data_len, src, des, 0, in_q);
    trans_delay = trans_delay + DIFS + back_off; //add the difs and backoff to trans delay
    gel.push_back(difs_timer);  //insert difs timer event in gel
    gel.sort(compare);
  }
}


int main(int argc, char** argv)
{
  /************************** Initialization *******************************/
  double arrival_rate ; //lambda
  int host_num;

  if (argc == 3)
  {
    arrival_rate = stod(argv[1]); //get lambda from user
    host_num = stoi(argv[2]);    //get number of hosts from user
  }else{
    cout << "ERROR: not 2 arguments" << endl;
    return 0;
  }

  cout << "Arrival rate (lambda) = " << arrival_rate << endl;
  cout << "Number of Hosts = " << host_num << endl << endl;

  double time = 0; //init time to 0

 //create all hosts and insert in array
  Host host_array[host_num];
  for (int i = 0; i < host_num; i++)
  {
    Host host_obj;
    host_obj.length = 0;
    host_array[i] = host_obj;
  }

  //create first arrival events for all hosts
  for (int i = 0; i < host_num; i++)
  {
    int frame_len = frame_len_gen();
    double trans_t = (frame_len * 8 * 1000) / channel_cap;
    Event e1(neg_exp_dis_time(arrival_rate), trans_t, 1,
             frame_len, i, pick_dest(i, host_num), 0, 0);
    gel.push_back(e1);
  }

  gel.sort(compare); //sort the gel

  /********************** End of Initialization **************************/

  for (int i = 0; i < 100000; i++)
  {
    Event event = gel.front();
    if (!gel.empty()){
      gel.pop_front();  //pop the first event
    }

    if (event.event_type == 1)  //if event is arrival
    {

        time = event.event_time; //update current time

        /* schedule the next arrival event */
        int frame_len = frame_len_gen();
        double trans_t = (frame_len * 8 * 1000) / channel_cap;
        Event new_av_event(time + neg_exp_dis_time(arrival_rate), trans_t, 1,
                           frame_len, event.source_host,
                           pick_dest(event.source_host, host_num), 0, 0);

        gel.push_back(new_av_event);  //insert new arrival in gel
        gel.sort(compare);

        /* Process the arrival event */
        event.enqueue_time = time;
        host_array[event.source_host].buffer.push(event); //insert packet in Host buffer
        host_array[event.source_host].length++; //increment host buffer length
        packets++;


        if (host_array[event.source_host].length == 1) //if first packet in buffer
        {
          //start difs timer only if first element in buffer
          start_difs(time, event.transmission_time, event.source_host,
                     event.dest_host, event.frame_length, time);
        }

      }

      if (event.event_type == 3) //if event is a DIFS timer
      {
          time = event.event_time; //update current time
          //difs timer has expired

          if (busy == 0)  // If channel is idle
          {
            //start transmitting packet
            /* schedule the receiving event */
            busy = 1; //channel is busy now
            Event recv_event(time + event.transmission_time, event.transmission_time,
                   2, event.frame_length, event.source_host, event.dest_host, 0, event.enqueue_time); //create receiving event.
                                         //the time it is arriving at dest host.

            //recalculate transmission time with channel cap of 10Mbps
            double tt = (event.transmission_time * channel_cap) / 10000000;

            trans_delay = trans_delay + tt ; // add transmission time to trans delay
            gel.push_back(recv_event); //insert receiving event in gel
            gel.sort(compare);

          }else{  // else channel is busy
            collisions++;
            int back_off = backoff(collisions);
            Event difs_timer(time + DIFS + back_off , event.transmission_time,
                             3, event.frame_length, event.source_host, event.dest_host, 0, event.enqueue_time); //create DIFS event
            trans_delay = trans_delay + DIFS + back_off;
            gel.push_back(difs_timer);  //insert difs timer event in gel
            gel.sort(compare);
          }

      }

      if(event.event_type == 2) //if event is a receiving event
      {
        time = event.event_time; //update current time
        //Packet transmission has ended
        busy = 0; //channel is idle
        //calculate the timeout for ack.
        double timeout = time + (1.5 * (ACK_TRANS + SIFS));

        //create sifs timer event
        Event sifs_timer(time + SIFS, event.transmission_time, 4, event.frame_length, event.source_host,
                         event.dest_host, timeout, event.enqueue_time);
        trans_delay = trans_delay + SIFS;
        gel.push_back(sifs_timer);  //insert SIFS timer event in gel
        gel.sort(compare);

      }

      if(event.event_type == 4) //if event is a sifs timer
      {
        time = event.event_time; //update current time
        //sifs timer has expired

        if (busy == 0) //if channel is idle
        {
          busy = 1; //channel is busy
          total_bytes = total_bytes + event.frame_length + 64; //update bytes transmitted

          Event ack(time + ACK_TRANS, 0, 5, 0, event.dest_host,
                    event.source_host, 0, event.enqueue_time); //create ack event
          trans_delay = trans_delay + 0.0512; //0.0512 = ack trans for channel of 10Mbps
          gel.push_back(ack); //insert ack event in gel
          gel.sort(compare);

        }else{ //else channel is busy
          collisions++;
          //Ack lost have to retransmit packet

          //create ack timeout event.
          Event ack_timeout(event.timeout, event.transmission_time, 6, 0,
                             event.source_host, event.dest_host, 0, event.enqueue_time);
          gel.push_back(ack_timeout);  //insert ack timeout event in gel
          gel.sort(compare);

        }
      }

      if (event.event_type == 5) //if event is an ack event
      {
        time = event.event_time; //update current time
        //ack has arrived to sender
        busy = 0; //channelis free(idle)

        // time is when packet comes out of the queue
        queue_delay = time - event.enqueue_time;

        //remove transmitted packet from host buffer
        if (host_array[event.dest_host].buffer.size() > 0 ){
          host_array[event.dest_host].buffer.pop();
          host_array[event.dest_host].length--; //decrement host buffer length
        }else{
          cout << "ERROR POP WITH BUFFER SIZE ZERO" << endl;
        }

        //Process next packet in buffer if any
        if(host_array[event.dest_host].length > 0 )
        {
          if (host_array[event.dest_host].buffer.size() > 0 ){
            Event next_packet =  host_array[event.dest_host].buffer.front();
            //start difs timer for next packet
            start_difs(time, next_packet.transmission_time, next_packet.source_host,
                       next_packet.dest_host, event.frame_length, next_packet.enqueue_time);
          }else{
            cout << "ERROR FRONT WITH BUFFER SIZE ZERO" << endl;
          }


        }

      }

      if (event.event_type == 6) //if event is ack timeout
      {
        time = event.event_time; //update current time
        //ack timeout has expired.

        //now need to retransmit packet
        start_difs(time, event.transmission_time, event.source_host,
                  event.dest_host, event.frame_length, event.enqueue_time);
      }
  }


  /*********************** Calculate Statistics **********************/
  int throughput = total_bytes / time;
  long double network_delay = (queue_delay + trans_delay) / packets;

  cout << "Throughput = " << throughput << endl;
  cout << "Network Delay = " << network_delay << endl;

  return 0;
}
