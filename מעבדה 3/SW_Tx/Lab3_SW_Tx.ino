// Stop & Wait Transmiter
// ----------------------------------------------------------------------------------------------------------------------------------------------------------
#include "EthernetLab.h"                                                   // Import Ethernet protocol functions  
#define DataSize 200
#define frame_size DataSize+5
#define R 10                                                               // Arduino's Bandwidth [Bytes/second]
enum state {                                                               // Transmiter Finite-State Machine
  Transmit,
  Wait,
  End
};

// ----------------------------------------------------------------------------------------------------------------------------------------------------------
// General porpuse variables:
// --------------------------
// Example DataSize = 200
char* Data = "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"; // Data To Transmit (Example)
// Example DataSize = 25
//char* Data = "1234567890123456789012345";
enum state State = Transmit;                                               // Init state variable
int payload_size = frame_size - 5;
int ack_recieved = 1;
char* frame = 0;
char* ack_payload = 0;
unsigned int data_index = 0;
int prev_ack = 1;
int curr_ack = 0;

// Time Measurement variables:
// ---------------------------
int measure_flag = 0;
int measure_taken = 0;
int error_prob_flag = 0;
unsigned int curr_clk = 0;                                                 // Current time variable
unsigned int prev_clk = 0;                                                 // Previous time variable
float RTT_sum = 0;                                                         // RoundTripTime Sum variable
unsigned int RTT_curr = 0;
unsigned int RTT_cnt = 0;                                                  // RoundTripTime Counter
float TimeOut;                                                             // TimeOut bound variable
float t_time = ((DataSize+frame_size)*8/R);                                // Transmit Time formula
float Transmit_Time = t_time/1000;

float Period_Time = 0;                                                     // Period Time = Transmit Time + RTT (Will be measured & updated in setup)
float S;                                                                   // Efficiency var

// Cyclic Redundancy Check - 32 bit:
// ---------------------------------
uint32_t CRC32 = 0;                                                        // CRC-32 variable - 4 bytes.
// ----------------------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  // Config
  Serial.begin(115200);
  setAddress(0, 20);                                                       // Set device as Transmiter - '0' - Tx, '1' - Rx, 20 - Pair number
  frame = new char[frame_size + 1];
  frame[frame_size] = '\0';
  prev_clk = 0;                                                            // Resets previous clock variable
  curr_clk = 1;                                                            // Resets current clock variable
  //1,10,100,1000,2000,3500,5000
  setDelay(1000);
}

void loop() {

  //  RoundTimeTrip Measurement:
  if (measure_flag == 0) {

    if (sendPackage(frame, frame_size) != 1)
      return;
    if (measure_taken == 0) {
      prev_clk = millis();
      measure_taken = 1;
    }
    if (readPackage(frame, frame_size) != 1)
      return;
      
    measure_flag = 1;
    curr_clk = millis();
    RTT_cnt++;
    RTT_sum = curr_clk - prev_clk;
    Period_Time = Transmit_Time + RTT_sum;                                 // Calculate & update Period_Time
    TimeOut = Period_Time + 5;                                             // Increase Period Time by 5ms and set the result as TimeOut bound.
    S = (Transmit_Time / Period_Time)*1000;                                // Efficiency formula

    Serial.print("RoundTimeTrip:");                                        // First Measurement Test Result
    Serial.println(RTT_sum);                                              
    Serial.print("Transmit Time:");
    Serial.println(Transmit_Time);
    Serial.print("Timeout bound:");
    Serial.println(TimeOut);
    Serial.print("Cycle Time: ");
    Serial.println(Period_Time);
    Serial.print("Efficiency - (S): ");
    Serial.println(S);
    measure_taken = 0;
  }

  switch (State) {
    case (Transmit): {
      // State "Transmit" - Transmit data frame.
        if (ack_recieved == 1) {
          for (int i = 0; i <= payload_size; i++) {
            if (i + data_index >= DataSize) {
              if (i == payload_size)
                frame[i] = char(48 + curr_ack);
              else
                frame[i] = '0';
            }
            else {
              if (i == payload_size) {
                frame[i] = char(48 + curr_ack);
              }
              else
                frame[i] = Data[i + data_index];
            }
          }
          CRC32 = calculateCRC(frame, payload_size + 1);
          // Place calculated CRC32 4 lsb bytes 
          for (int j = 1; j < 5; j++) {
            frame[frame_size - j] = char(CRC32 % 256);
            CRC32 = CRC32 >> 8;
          }
          Serial.println("----------------------------------------------------------------");
          Serial.print("Frame: ");
          for (int m = 0; m < frame_size; m++) {
            if (m == payload_size)
              Serial.print("|Ack:");
            if (m == payload_size + 1)
              Serial.print("|CRC:");
            Serial.print(frame[m]);
          }
          Serial.println();
          ack_recieved = 0;
        }
        if (measure_taken == 0) {
          prev_clk = millis();
          measure_taken = 1;
        }
        // Simulate Data Error
        //if (error_prob_flag == 0){
        //  frame[3] = 0;  // <--- ERROR PROBABILITY CHECK
        //  error_prob_flag = 1;
        //}
        if ((sendPackage(frame, frame_size)) == 1 && ack_recieved == 0) {
          State = Wait;
          measure_taken = 0;
        }
        break;
      }
    case (Wait): {
        // State "Wait" - Waiting for ack frame to recieve.
        curr_clk = millis();
        if (curr_clk - prev_clk < TimeOut) {
          ack_payload = new char[2];
          if (readPackage(ack_payload, 2) == 1) {
            if ((ack_payload[0] == '0' && curr_ack == 1) || (ack_payload[0] == '1' && curr_ack == 0)) {
              curr_clk = millis();
              ack_recieved = 1;
              Serial.print("Duration: ");
              Serial.println(curr_clk - prev_clk);
              RTT_sum += (curr_clk - prev_clk);
              RTT_cnt++;
              Serial.print("Ack ");
              Serial.print(curr_ack);
              Serial.println(" recieved");
              prev_ack = curr_ack;
              curr_ack = 1 - curr_ack;

              data_index += payload_size;
            }
          }
          delete[] ack_payload;
        }
        else {
          if (data_index >= DataSize) {
            // State End condition
            Serial.println("----------------------------------------------------------------");
            Serial.println("End");
            Serial.print("Average RTT:");
            Serial.println(RTT_sum / RTT_cnt);
            delete[] frame;
            delete[] ack_payload;
            State = End;
          }
          else {
            State = Transmit;
            ack_recieved = 1;
          }
          break;
        }
      case (End): {
        // State "End" - Used only for report measurements
          break;
        }
      }
  }
}
