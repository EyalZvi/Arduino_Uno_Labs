// GO BACK N Transmiter
// ----------------------------------------------------------------------------------------------------------------------------------------------------------
#include "EthernetLab.h"                                              // Import Ethernet protocol functions  
#define DataSize 25
#define N 6                                                           // N defined as 6
#define R 10                                                          // Arduino's Bandwidth [Bytes/second]
enum state {                                                          // Transmiter Finite-State Machine
  Transmit,
  Wait,
  End
};

// ----------------------------------------------------------------------------------------------------------------------------------------------------------
// General porpuse variables:
// --------------------------
// Example DataSize = 200
//char* Data = "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890"; // Data To Transmit (Example)
// Example DataSize = 25
char* Data = "1234567890123456789012345";
enum state State = Transmit;                                          // Init state variable
int frame_size;
int payload_size;
int ack_recieved = 1;
char* frame = 0;
char* ack_payload = 0;
int prev_ack = N - 1;
int curr_ack = 0;
int frames_sent = 0;
float frames_to_send_calc = float(DataSize) / float(payload_size);
int frames_to_send = N;

// Time Measurement variables:
// ---------------------------
int measure_flag = 0;
int measure_taken = 0;
int error_prob_flag = 0;
unsigned int curr_clk = 0;                                            // Current time variable
unsigned int prev_clk = 0;                                            // Previous time variable
float RTT_sum = 0;                                                    // RoundTripTime Sum variable
unsigned int RTT_curr = 0;
unsigned int RTT_cnt = 0;                                             // RoundTripTime Counter
float TimeOut;                                                        // TimeOut bound variable
float t_time = (((DataSize + frame_size) * 8) / (R));                 // Transmit Time formula
float Transmit_Time = t_time / 1000;
float Period_Time = 0;                                                // Period Time = Transmit Time + RTT (Will be measured & updated in setup)
float S = 0;

// Cyclic Redundancy Check - 32 bit:
// ---------------------------------
uint32_t CRC32 = 0;                                                   // CRC-32 variable - 4 bytes.
// ----------------------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  // Config
  Serial.begin(115200);
  setAddress(0, 20); // Set device as Transmiter - '0' - Tx, '1' - Rx, 20 - Pair number
  frame_size = DataSize + 5;
  payload_size = frame_size - 5;
  frame = new char[frame_size + 1];
  frame[frame_size] = '\0';
  payload_size = frame_size - 5;
  prev_clk = 0;                                                       // Resets previous clock variable
  curr_clk = 1;                                                       // Resets current clock variable
  //setDelay(1);
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
    RTT_sum = N * (curr_clk - prev_clk);
    Period_Time = N * Transmit_Time + RTT_sum;                        // Calculate & update Period_Time
    TimeOut = Period_Time * 1.25;                                     // Increase Period Time by 50% and set the result as TimeOut bound.
    S = 1000 * N * Transmit_Time / Period_Time;                       // Efficiency formula

    Serial.print("RoundTimeTrip:");                                   // First Measurement Test Result
    Serial.println(RTT_sum / N);                                     
    Serial.print("Transmit Time:");
    Serial.println(Transmit_Time);
    Serial.print("Window Timeout bound:");
    Serial.println(TimeOut);
    Serial.print("Cycle Time: ");
    Serial.println(Period_Time);
    Serial.print("Efficiency - (S): ");
    Serial.println(S);
  }

  switch (State) {
    case (Transmit): {
      // State "Transmit" - Transmit data frame.
        if (ack_recieved == 1) {
          if (frames_to_send != frames_sent) {
            for (int i=0; i<=payload_size; i++) {
              if (i == payload_size) {
                frame[i] = char(48 + (curr_ack + frames_sent) % N);
                break;
              }
              frame[i] = Data[i];
            }
          CRC32 = calculateCRC(frame, payload_size + 1);
          // Place calculated CRC32 4 lsb bytes 
          for (int j = 1; j < 5; j++) {
            frame[frame_size - j] = char(CRC32 % 256);
            CRC32 = CRC32 >> 8;
          }
        }
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
        if (sendPackage(frame, frame_size) == 1) {
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
          frames_sent++;
          if (frames_sent % N == 0 || frames_to_send == frames_sent) {
            State = Wait;
            ack_recieved = 0;
          }
        }
        break;
      }
    case (Wait): {
        // State "Wait" - Waiting for ack frame to recieve.
        curr_clk = millis();
        if (curr_clk - prev_clk < TimeOut) {
          ack_payload = new char[2];
          if (readPackage(ack_payload, 2) == 1) {
            if ((int(ack_payload[0]) - 48) >= curr_ack) {
              curr_clk = millis();
              measure_taken = 0;
              ack_recieved = 1;
              Serial.println("----------------------------------------------------------------");
              Serial.print("Window Duration: ");
              Serial.println(curr_clk - prev_clk);
              RTT_sum += (curr_clk - prev_clk);
              RTT_cnt++;
              prev_ack = curr_ack;
              curr_ack = (int(ack_payload[0]) - 48) % N ;
              Serial.print("Ack ");
              Serial.print(int(ack_payload[0]) - 48);
              Serial.println(" recieved");
              ack_recieved = 1;
            }
          }
        }
        delete[] ack_payload;
        if (frames_to_send == frames_sent) {
          // State End condition
          Serial.println("----------------------------------------------------------------");
          Serial.println("End");
          Serial.print("Average RTT:");
          Serial.println(RTT_sum / (RTT_cnt * N));
          frames_sent = 0;
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
