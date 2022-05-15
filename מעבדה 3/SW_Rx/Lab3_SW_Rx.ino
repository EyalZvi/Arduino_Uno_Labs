// Stop & Wait Reciever
// ----------------------------------------------------------------------------------------------------------------------------------------------------------
#include "EthernetLab.h"                                              // Import Ethernet protocol functions
#define DataSize 200                                                  // Define DataSize in bytes                                                        // Arduino's Bandwidth [Bytes/second]
#define frame_size DataSize+5                                         // Frame Size
enum state {                                                          // Reciever Finite-State Machine
  Wait,
  Recieved,
  End
};

// ----------------------------------------------------------------------------------------------------------------------------------------------------------
// General porpuse variables:
// --------------------------
enum state State = Wait;                                              // Init state variable
int payload_size = frame_size - 5;
int payload_cnt =  0;
int prev_ack = 0;
int curr_ack = 1;
char Data[DataSize];
char frame[frame_size + 1];
char ack_payload[2] = {0};
int measure_flag = 0;
float bad_packets = 0;
float total_packets = 0;

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
  ack_payload[1] = '\0';
}

void loop() {

  // RoundTimeTrip First Measurement:
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
          Serial.println("End");
          Serial.print("Error Probability: ");
          Serial.println(bad_packets / total_packets);
          break;
        }
        if (readPackage(frame, frame_size) == 1 && frame[0] != NULL) {
          total_packets++;
          if ((frame[payload_size] == '0' && curr_ack == 1) || (frame[payload_size ] == '1' && curr_ack == 0)) {
            Serial.println(frame);
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
              State = Recieved;
              ack_payload[0] = char(curr_ack + 48);
              data_index += payload_size;
            }
            else{
              bad_packets++;
            }
          }
          else{
            bad_packets++;
          }
        }
        break;
      }
    case (Recieved): {
      // State "Recieved" - Data frame recieved, should transmit an ack frame in response
        if (sendPackage(ack_payload, 2) == 1) {
          prev_ack = curr_ack;
          curr_ack = 1 - curr_ack;
          Serial.print("Ack ");
          Serial.print(ack_payload);
          Serial.print(" sent");
          Serial.println();
          State = Wait;
        }
        break;
      }
    case (End): {
        // State "End" - Used only for report measurements
        break;
      }
  }

}
