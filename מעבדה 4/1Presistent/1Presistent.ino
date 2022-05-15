// 1-Presistent
#define TIME_SLOT 200
#define prop_time 300
#define R 10
#include "EthernetLab4.h"
enum state {                                                          // Transmiter Finite-State Machine
  Transmit,
  Receive
};
enum state State = Transmit;

//Variables
char Ack[2];
int curr_sn = 0;
char* data = "Eyal and Saar";
int data_size = strlen(data);
int payload_size = data_size + 4;
char payload[19];
int curr_time;
int prev_time;
int sending = 0;
int failed_packets = 0;
int timeout_curr_time;
int timeout_prev_time;
int timeout = prop_time + ((data_size + payload_size) * 8) / (1000 * R)  ;

// CD - Local Time Variables
unsigned long rand_wait;
unsigned long cd_prev_time = 0;
unsigned long cd_curr_time = 0;

void setup() {
  // Configuration setup block
  Serial.begin(115200);
  setAddress(0, 20);
  setMode(PRES);
}

void loop() {
  switch (State) {
    case (Transmit): {                                                // Case 1: Transmit
        if (sending == 0) {
          if (checkLine() == 1) {
            for (int i = 0; i < payload_size; i++) {
              if (i < 4) {
                if (i == 0) {
                  payload[i] = char(0);
                  Serial.print("Type:");
                  Serial.print(char(int(payload[i]) + 48));          // Build the packet byte by byte and print to screen
                  Serial.print("|");                                 // *Only when sending = 0 and line is not busy
                }
                if (i == 1) {
                  payload[i] = char(curr_sn);
                  Serial.print("Ack:");
                  Serial.print(char(int(payload[i]) + 48));
                  Serial.print("|");
                }
                if (i == 2) {
                  payload[i] = char(data_size - (data_size % 16));
                  Serial.print("DataLen:");
                }
                if (i == 3) {
                  payload[i] = char(data_size % 16);
                  Serial.print(int(payload[i]) + int(payload[i - 1]) * 10);
                  Serial.print("|Data:");
                }
              }
              else {
                payload[i] = data[i - 4];
                Serial.print(payload[i]);
              }
            }
            Serial.println();                                       // Start the package sending and sample current time, flip sending flag ON
            startPackage(payload, payload_size);
            prev_time = millis();
            sending = 1;
          }
          else {
            break;
          }
        }
        curr_time = millis();
        if (curr_time - prev_time >= prop_time) {                    // Check if prop_time has passed,
          endPackage(1);                                             // if passed no collision occured: reset failed packets , wait for Ack
          timeout_prev_time = millis();
          sending = 0;
          failed_packets = 0;
          State = Receive;
          break;
        }
        else {
          if (checkLine() == 0) {                                   //  Check if the line is busy (Collision)
            endPackage(0);                                          //  if line is busy collision occured: inc. failed packets, enter collision func, then send package again
            sending = 0;
            failed_packets++;
            cd_func();
          }
        }
        break;
      }
    case (Receive): {
        timeout_curr_time = millis();
        if (timeout_curr_time - timeout_prev_time >= timeout) {
          Serial.println("Timeout exceeded, Resend package");
          Serial.println("--------------------------------------------");
          State = Transmit;
          break;
        }
        if (readPackage(Ack, 2) == 1) {                              // Case 2: Receive
          if (int(Ack[0]) == 1 && int(Ack[1]) == 1 - curr_sn) {
            Serial.print("Ack ");                                    // if a packet with Type = 1 recieved AND Ack is correct, Ack recieved:
            Serial.print(char(Ack[1] + 48));                         // flip current Ack and countinue on to send a new packet
            Serial.println(" received");
            Serial.println("--------------------------------------------");
            curr_sn = 1 - curr_sn;
            State = Transmit;
          }
        }
        break;
      }
  }
}

void cd_func() {
  // Collision Detection Function
  Serial.print("Collision Detected!, failed packets: ");             // In case of collision cd_func is activated
  Serial.println(failed_packets);
  if (failed_packets < 7)                                            // Generate a randon time by Exp backoff (according to failed packets) then wait it out
    rand_wait = random(0, (8 << failed_packets) - 1);
  else
    rand_wait = random(0, 1023);
  rand_wait *= TIME_SLOT;
  cd_curr_time = millis();
  cd_prev_time = millis();
  Serial.print("Waiting Time:");
  Serial.println(rand_wait);
  Serial.println("--------------------------------------------");
  while ((cd_curr_time - cd_prev_time) < rand_wait) {               // Time-Waiting loop
    // Valid time condition
    cd_curr_time = millis();
  }
  return;
}
