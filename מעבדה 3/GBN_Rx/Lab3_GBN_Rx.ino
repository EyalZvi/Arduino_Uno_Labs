// GO BACK N Reciever
// ----------------------------------------------------------------------------------------------------------------------------------------------------------
#include "EthernetLab.h"                                              // Import Ethernet protocol functions
#define N 6                                                           // N defined as 6
#define DataSize 25*N                                                 // Define DataSize in bytes
#define R 10                                                          // Arduino's Bandwidth [Bytes/second]
enum state {                                                          // Reciever Finite-State Machine
  Wait,
  Recieved,
  End
};

// ----------------------------------------------------------------------------------------------------------------------------------------------------------
// General porpuse variables:
// --------------------------
enum state State = Wait;                                              // Init state variable
int frame_size;
int payload_size;
int payload_cnt =  0;   
int prev_ack = 5;
int curr_ack = 0;
char Data[DataSize];
char ack_payload[2] = {0};
char* frame = 0;
int measure_flag = 0;
float bad_packets = 0;
float total_packets = 0;
int frames_recieved = 0;
int frames_to_recieve = N;
int frames_recieved_in_window = 0;

// Cyclic Redundancy Check - 32 bit:
// ---------------------------------
char* CRC32[4];                                                       // CRC-32 variable - 4 bytes.
uint32_t CRC32_recieved = 0;
uint32_t CRC32_calc = 0;
// ----------------------------------------------------------------------------------------------------------------------------------------------------------

int data_index = 0;

void setup() {
  // Config
  Serial.begin(115200);
  setAddress(1, 20);                                                  // Set device as Reciever - ['1' - Rx, '0' - Tx, 20 - Pair number]
  ack_payload[2] = '\0';
  frame_size = (DataSize / N) + 5;
  payload_size = frame_size - 5;
  frame = new char[frame_size + 1];
}

void loop() {

  // RoundTimeTrip Measurement:
  if (measure_flag == 0) {
    if (readPackage(frame, frame_size) != 1)
      return;
    if (sendPackage(frame, frame_size) != 1)
      return;
    measure_flag = 1;
  }


  switch (State) {
    case (Wait): {
      // State "Wait" - Waiting for data frame to recieve.
        if (data_index >= DataSize) {
          // State End Condition
          State = End;
          delete[] frame;
          Serial.println("End");
          Serial.print("Error Probability: ");
          Serial.println(bad_packets / total_packets);
          break;
        }
        if (readPackage(frame, frame_size) == 1 && frame[0] != NULL ) {
          total_packets++;
          if ((int(ack_payload[0]) + 48) % N == curr_ack) {
            for (int k = 0; k < frame_size; k++) {
              if (k == payload_size)
                Serial.print("|ACK:");
              if (k == payload_size + 1)
                Serial.print("|CRC:");
              Serial.print(frame[k]);
            }
            Serial.println();
            CRC32_recieved = calculateCRC(frame, payload_size + 1);
            for (int i = frame_size - 5; i <= frame_size - 1 ; i++) {
              if (int(frame[i]) >= 0) {
                // CRC32 convertion from char back to int
                CRC32_calc += int(frame[i]);
              }
              else {
                CRC32_calc += (int(frame[i]) + 256);
              }
              if ( i != frame_size - 1)
                CRC32_calc = CRC32_calc << 8;
            }
            if (CRC32_calc == CRC32_recieved) {
              // Verify valid CRC32
              payload_cnt++;
              data_index += payload_size;
              prev_ack = curr_ack;
              curr_ack = curr_ack + 1;
              ack_payload[0] = char(curr_ack + 48);
              curr_ack = curr_ack % N;
              frames_recieved++;
              if (frames_recieved == N || (frames_to_recieve < N && frames_recieved == frames_to_recieve)) {
                State = Recieved;
                frames_to_recieve -= frames_recieved;
                frames_recieved = 0;
              }
            }
            else {
              bad_packets++;
            }
          }
          else {
            bad_packets++;
          }
        }
        break;
      }
    case (Recieved): {
      // State "Recieved" - Data frame recieved, should transmit an ack frame in response
        if (sendPackage(ack_payload, 2) == 1) {
          Serial.print("Ack ");
          Serial.print( ack_payload[0]);
          Serial.println(" sent");
          State = Wait;
        }
        break;
      }
    case (End): {
        // State "End" - Used only for report measurement
        break;
      }
  }

}
