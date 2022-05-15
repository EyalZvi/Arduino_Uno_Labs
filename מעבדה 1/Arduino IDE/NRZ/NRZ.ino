// Define Rate consts
#define bit_time 1000
#define samp_num 5
void setup()
{
  // Config Block
  pinMode(7, OUTPUT);
  pinMode(8, INPUT);
  pinMode(11, OUTPUT);
  Serial.begin(9600);
}

//  Trasnmiter global variables
unsigned int T_curr_time = 0;                                     // Tx timestamps variables
unsigned int T_ref_time = 0;
int bit_send = 0;                                                 // Current bit to transmit
int i = 1;                                                        // Current transmiting bit index

//  Reciever global variables
unsigned int R_curr_time = 0;                                     // Rx timestamps variables
unsigned int R_ref_time = 0;
int curr_bit = 0;                                                 // Current recieved bit
int cycle_cnt = 0;                                                // Cycle counter var.
int msg = 0;                                                      // Total message var.
int shift = 1;                                                    // Shift process var.
int j = 0;                                                        // Current recieved bit index

void loop()
{
  // Priority Block:
  Tx();
  Rx();
}
void Tx() {
  // Transmitter Block:
  T_curr_time = millis();                                         // Sample Current time
  if (T_curr_time - T_ref_time >= bit_time) {                     // Branch according to bit_time
    Serial.print(i);
    Serial.print(")");
    Serial.print("TX ->>");
    T_ref_time = T_curr_time;                                     // Update Tx reference time var
    bit_send = random(0, 2);                                      // Generate random bit value
    Serial.print(" Send ");
    Serial.println(bit_send);
    digitalWrite(7, bit_send);                                    // Transmit generated bit
    i++;                                                          // Current Transmiting bit index inc
  }
}

void Rx() {
  // Reciever Block:
  R_curr_time = millis();                                         // Sample Current time
  if (R_curr_time - R_ref_time >= bit_time / samp_num) {          // Branch according to bit time div. samples number
    R_ref_time = R_curr_time;                                     // Update Rx reference time var
    curr_bit = digitalRead(8);                                    // Read trasmitted bit
    msg = msg + curr_bit * shift;                                 // Update current bit to msg representation
    if ( cycle_cnt == samp_num - 1) {
      msg = (msg & 14) >> 1;                                      // in last cycle normalize to 3 bit
      Serial.print(j);
      Serial.print(")");
      j++;
      if (msg == 0){                                              // By checking the 3 bit msg we convert to '1'/'0'/Error
        Serial.println("RX <<- Recieved: 0");
      digitalWrite(11, LOW);
      }
      if (msg == 7){
        Serial.println("RX <<- Recieved: 1");
      digitalWrite(11, HIGH);
      }
      if (msg != 7 && msg != 0)
        Serial.println("RX <<- Error - no bit recieved");
      msg = 0;                                                    // Reset variables after last cycle
      cycle_cnt = 0;
      shift = 1;
      return;
    }
    cycle_cnt++;                                                  // Cycle counter inc.
    shift = shift << 1;                                           // Update shl_helper variable
  }
}
