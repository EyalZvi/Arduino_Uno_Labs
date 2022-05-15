// Define Rate consts
#define bit_time 1000                                             // Bit Time define
#define samp_num 5                                                // Bit time division define
#define Data_len 8                                                // Data length define
#define RX_PIN 2
#define TX_PIN 3
enum State {
  Idle,
  Start_bit,
  Data_bit,
  Parity_bit,
  Stop_bit,
};

//  Trasnmiter global variables
unsigned int T_curr_time = 0;                                     // Tx timestamps variables
unsigned int T_ref_time = 0;
unsigned int T_pairty_bit = 0;                                      // Parity bit calculation and transmit
int bit_send = 0;                                                 // Current bit to transmit
int T_data_cnt = 0;                                               // Tx Number of current data bit
int idle_cnt = random(1, 3);                                      // Random 1's before start bit

enum State T_state;                                               // Current transmitter state

//  Reciever global variables
unsigned int R_curr_time = 0;                                     // Rx timestamps variables
unsigned int R_ref_time = 0;
unsigned int R_pairity_bit = 0;                                   // Pairty bit validation var
unsigned int recieved = 0;                                        // Recieved bit var
int curr_sample = 0;                                              // Current recieved sample
int cycle_cnt = 0;                                                // Cycle counter var
int msg = 0;                                                      // Total message var
int shift = 1;                                                    // Shift process var

int R_data_cnt = 0;                                               // Rx Number of current data bit
int curr_bit = 0;                                                 // Current sampled bit var
int error_flag = 0;                                               // Error flag var
int R_Shift = 1;                                                  // Reciever shift var
int idle_prev_bit = 0;                                            // Idle state previous bit var
int idle_curr_bit = 0;                                            // Idle state current bit var

enum State R_state;                                               // Current reciever state

void setup()
{
  // Config Block
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
  Serial.begin(9600);

}

void loop()
{
  // Priority Block:
  uart_tx();
  uart_rx();
}
void uart_tx() {
  // Transmitter Block:
  T_curr_time = millis();                                         // Sample Current time
  if (T_curr_time - T_ref_time >= bit_time) {                     // Branch according to bit_time
    Serial.print("TX ->>");
    T_ref_time = T_curr_time;                                     // Update Tx reference time var


    switch (T_state) {

      case Idle: {                                                // Idle State - stabilize transmission
          bit_send = 1;
          Serial.print("T CASE IDLE |");
          Serial.print(" Bit:");
          Serial.println(bit_send);
          digitalWrite(TX_PIN, bit_send);
          idle_cnt -= 1;
          if (idle_cnt == 0) {
            T_state = Start_bit;
            idle_cnt = random(1, 3);
          }
          break;
        }
      case Start_bit: {                                           // Start_bit State - Prepare to transmit
          bit_send = 0;
          Serial.print("T CASE START |");
          Serial.print(" Bit:");
          Serial.println(bit_send);
          digitalWrite(TX_PIN, bit_send);
          T_state = Data_bit;
          break;
        }
      case Data_bit: {                                            // Data_bit State - Data transfer
          bit_send = random(0,2);
          Serial.print("T CASE DATA");
          digitalWrite(TX_PIN, bit_send);
          T_pairty_bit += bit_send;
          T_data_cnt++;
          Serial.print(" - ");
          Serial.print(T_data_cnt);
          Serial.print("| Bit:");
          Serial.println(bit_send);
          if (T_data_cnt == Data_len) {
            T_state = Parity_bit;
            T_data_cnt = 0;
          }
          break;
        }
      case Parity_bit: {                                        // Pairity_bit State - Error correction check
          bit_send = !(T_pairty_bit % 2);
          Serial.print("T CASE PARITY |");
          Serial.print(" Bit:");
          Serial.println(bit_send);
          digitalWrite(TX_PIN, bit_send);
          T_state = Stop_bit;
          T_pairty_bit = 0;
          break;
        }
      case Stop_bit: {                                          // Stop_bit State - End of current transmission
          bit_send = 1;
          Serial.print("T CASE STOP |");
          Serial.print(" Bit:");
          Serial.println(bit_send);
          digitalWrite(TX_PIN, bit_send);
          T_state = Idle;
          break;
        }
    }
  }
}

void uart_rx() {
  // Reciever Block:
  R_curr_time = millis();                                         // Sample Current time
  if (R_curr_time - R_ref_time >= bit_time / samp_num) {          // Branch according to bit time div. samples number
    R_ref_time = R_curr_time;                                          // Update Rx reference time var
    curr_sample = digitalRead(RX_PIN);                            // Read trasmitted bit
    msg = msg + curr_sample * shift;                              // Update current bit to msg representation
    if ( cycle_cnt == samp_num - 1) {
      msg = (msg & 14) >> 1;                                      // in last cycle normalize to 3 bit
      if (msg == 0) {                                             // By checking the 3 bit msg we convert to '1'/'0'/Error
        curr_bit = 0;
      }
      if (msg == 7) {
        curr_bit = 1;
      }
      if (msg != 7 && msg != 0) {
        error_flag = 1;
      }
      msg = 0;                                                    // Reset variables after last cycle
      cycle_cnt = 0;
      shift = 1;
    }
    else {
      cycle_cnt++;                                                // Cycle counter inc.
      shift = shift << 1;                                         // Update shl_helper variable
      return;
    }

    if (cycle_cnt == 0) {
      //      Serial.print("RX <<-");
      switch (R_state) {
        
        case Idle: {                                              // Idle State - Waiting for Start_bit
//          Serial.print("RX <<- CASE IDLE");
//          Serial.print(" Bit:");
//          Serial.println(curr_bit);
            idle_prev_bit = idle_curr_bit;
            idle_curr_bit = curr_bit;
            if (idle_prev_bit == 1 && idle_curr_bit == 0)
              R_state = Start_bit;
            else
              break;
          }

        case Start_bit: {                                         // Start_bit State - Prepare to recieve
            //            Serial.print("R CASE START");
            //            Serial.print(" Bit:");
            //            Serial.println(curr_bit);
            R_state = Data_bit;
            break;
          }
        case Data_bit: {                                          // Data_bit State - Data collecting
             //           Serial.print("R CASE DATA");
            R_pairity_bit += curr_bit;
            R_data_cnt++;
            //            Serial.print(" - ");
            //            Serial.print(R_bit_cnt);
            //            Serial.print(" Bit:");
            //            Serial.println(curr_bit);
            recieved = recieved + curr_bit * R_Shift;
            R_Shift = R_Shift << 1;
            if (R_data_cnt == Data_len) {
              R_state = Parity_bit;
              R_Shift = 1;
              R_data_cnt = 0;
            }
            break;
          }
        case Parity_bit: {                                        // Pairity_bit State - Error correction check
            if (curr_bit != (!(R_pairity_bit % 2)))
              error_flag = 1;
             //            Serial.print("R CASE PARITY");
            //             Serial.print(" Bit:");
            //             Serial.println(curr_bit);
            R_state = Stop_bit;
            R_pairity_bit = 0;
            break;
          }
        case Stop_bit: {                                          // Stop_bit State - End of current session
            if (curr_bit != 1)
              error_flag = 1;
            Serial.print("Rx <<- Reading... ");
            //  Serial.print(" Bit:");
            // Serial.println(curr_bit);
            if (error_flag == 0) {
              Serial.print("Word recieved: ");
              Serial.println(recieved, BIN);
            }
            else {
              Serial.println("Error detected, data thrown");
            }
            idle_prev_bit = 0;
            idle_curr_bit = 1;
            R_state = Idle;
            recieved = 0;
            error_flag = 0;
            break;
          }
      }

    }
  }
}
