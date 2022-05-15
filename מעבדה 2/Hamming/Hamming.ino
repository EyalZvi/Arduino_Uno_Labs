#define RX_PIN 2
#define TX_PIN 3
#define RX_CLK 7
#define TX_CLK 8                                                // Defines: In/Out pins and data length
#define clk_interval 1000
#define data_len 14

//  Trasnmiter global variables
unsigned int curr_time = 0;                                     // Tx timestamps variables
unsigned int ref_time = 0;
int T_bit_cnt = 0;                                              // Tx data bit counter

// Reciever global variables
unsigned int curr_clk = 0;                                      // Rx current clock var
unsigned int last_clk = 0;                                      // Rx previous clock var
int curr_bit;                                                   // Current sampled bit var
int recieved = 0;                                               // Recieved bit var
int R_bit_cnt = 0;                                              // Rx data bit counter
int shift = 1;                                                  // Shift var
int send_bit = 0;                                               // Transmitted bit var

// Hamming Encoder variables
char Data;                                                      // ASCII character to send
int Data_int;                                                   // int repr. of ASCII character
int Msg[14];                                                    // Array of 14 bits (2*7) to transmit through TX-USART (binary repr.)
int LSB_counter = 6;
int MSB_counter = 13;                                           // Counters for array use

// Hamming Decoder variables
int Collected[7];                                               // Array of 7 bits of recieved bits from RX-USART (binary repr.)
int decoded_words = 0;                                          // Counter of decoded words
int Error_Detector = 0;                                         // Generate E3E2E1 repr. as learned in Syndrome Decoding
int data_to_char = 0;                                           // Final number to convert into char output

//Global Flags
int Hamming_TX_Done = 0;                                        // Flags that indicate if Hamming function use is currently needed (Done or not)
int Hamming_RX_Done = 1;

int USART_flag = 0;
void setup() {

  // Config Block
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
  pinMode(TX_CLK, OUTPUT);
  pinMode(RX_CLK, INPUT);
  Serial.begin(9600);    
}

void loop() {                                                   // Main loop - We send a character in 2 packets of 7 bits with Hamming algorithm implemented

  if (Hamming_TX_Done == 0)
    Hamming4_tx();

  if(USART_flag == 1){
    usart_tx();
    usart_rx();
  }
    
  if (Hamming_RX_Done == 0)
    Hamming4_rx();
}

void usart_tx() {
  // Transmitter Block:
  curr_time = millis();                                          // Current time sampling
  if (curr_time - ref_time == clk_interval / 2) {                // Lower half clock interval - set clock on "Low"
    digitalWrite(TX_CLK, 0);
  }
  if (curr_time - ref_time == clk_interval) {                    // Higher half clock interval - set clock on "High"
    ref_time = curr_time;
    send_bit = Msg[data_len - 1 - T_bit_cnt];
    digitalWrite(TX_PIN, send_bit);
    digitalWrite(TX_CLK, 1);
    Serial.print("TX - Send bit #");                             // Print currently sent bit with index in word
    Serial.print(T_bit_cnt % 7);
    Serial.print(" | Value: ");
    Serial.println(send_bit);
    if (T_bit_cnt == data_len - 1) {
      T_bit_cnt = 0;
    }
    else
      T_bit_cnt ++;
  }
}

void usart_rx() {
  curr_clk = digitalRead(RX_CLK);
  if (curr_clk == 0 && last_clk == 1) {                          // Detecting 'Falling Edge'
    curr_bit = digitalRead(RX_PIN);
    recieved += curr_bit * shift;
    shift = shift << 1;
    R_bit_cnt ++;

    if (R_bit_cnt == data_len / 2) {                             // current session end-condition.
      Serial.print("Word recieved: ");
      Serial.println(recieved, BIN);                             // Print word recieved (as recieved before Hamming check)
      shift = 1;
      R_bit_cnt = 0;
      Hamming_RX_Done = 0;
    }
  }
  last_clk = curr_clk;
}

void Hamming4_tx() {
  Hamming_TX_Done = 1;                                           // Set flag to '1' so this section won't repeat itself
  Data = 'z';
  Data_int = int(Data);
  Serial.print("Send: '");
  Serial.print(Data);
  Serial.print("' - ");
  Serial.println(Data_int);

  //MSB: Data bits
  Msg[13] = Data_int % 2; // D4
  Data_int /= 2;
  Msg[12] = Data_int % 2; // D3
  Data_int /= 2;
  Msg[11] = Data_int % 2; // D2
  Data_int /= 2;
  Msg[9] = Data_int % 2;  // D1
  Data_int /= 2;
  // Setting the ground for Hamming - Filling the array with data bits
  //LSB: Data bits
  Msg[6] = Data_int % 2; // D4
  Data_int /= 2;
  Msg[5] = Data_int % 2; // D3
  Data_int /= 2;
  Msg[4] = Data_int % 2; // D2
  Data_int /= 2;
  Msg[2] = Data_int % 2; // D1

  //LSB: Parity bits
  Msg[0] = Msg[2] ^ Msg[4] ^ Msg[6]; // P1 = D1 XOR D2 XOR D4
  Msg[1] = Msg[2] ^ Msg[5] ^ Msg[6]; // P2 = D1 XOR D3 XOR D4
  Msg[3] = Msg[4] ^ Msg[5] ^ Msg[6]; // P3 = D2 XOR D3 XOR D4
  // Calculating parity bits and placing them in the array
  //MSB: Parity bits
  Msg[7] = Msg[9] ^ Msg[11] ^ Msg[13]; // P1 = D1 XOR D2 XOR D4
  Msg[8] = Msg[9] ^ Msg[12] ^ Msg[13]; // P2 = D1 XOR D3 XOR D4
  Msg[10] = Msg[11] ^ Msg[12] ^ Msg[13]; // P3 = D2 XOR D3 XOR D4

  USART_flag = 1;
}


void Hamming4_rx() {
  Hamming_RX_Done = 1;

    //Print
  Serial.print("Messege recieved as: ");
  Serial.println(recieved, BIN);

  //Data bits
  for (int i = 6; i >= 0; i--) {
    Collected[i] = recieved % 2;                               // Filling Collected array to implement Hamming algorithm
    recieved /= 2;
  }
  //Simulate_noise_1bit();
  //Simulate_noise_2bits();
  //Simulate_noise_3bits();
                                            
  Error_Detector = (Collected[2] + Collected[4] + Collected[6] + Collected[0]) % 2 + (((Collected[2] + Collected[5] + Collected[6] + Collected[1]) % 2) << 1)  + (((Collected[4] + Collected[5] + Collected[6] + Collected[3]) % 2) << 2);
  switch (Error_Detector) {                                                 
    case (0): {                                                             // E3E2E1 ^
        Serial.println("Packet recieved - No error detected");              // Making Error_Detector and choosing the right action in the Hamming algorithm (Longest line of code ever ^ )
        break;
      }
    case (1): {
        Collected[0] ^= 1;
        Serial.println("Packet recieved with an error in P1, Corrected.");
        break;
      }
    case (2): {
        Collected[1] ^= 1;
        Serial.println("Packet recieved with an error in P2, Corrected.");
        break;
      }
    case (3): {
        Collected[2] ^= 1;
        Serial.println("Packet recieved with an error in D1, Corrected.");
        break;
      }
    case (4): {
        Collected[3] ^= 1;
        Serial.println("Packet recieved with an error in P3, Corrected.");
        break;
      }
    case (5): {
        Collected[4] ^= 1;
        Serial.println("Packet recieved with an error in D2, Corrected.");
        break;
      }
    case (6): {
        Collected[5] ^= 1;
        Serial.println("Packet recieved with an error in D3, Corrected.");
        break;
      }
    case (7): {
        Collected[6] ^= 1;
        Serial.println("Packet recieved with an error in D4, Corrected.");
        break;
      }
  }

  data_to_char += Collected[6] << (0 + 4 * decoded_words);
  data_to_char += Collected[5] << (1 + 4 * decoded_words);                    //After Hamming applied add the bits at the right locations to get back to char representation
  data_to_char += Collected[4] << (2 + 4 * decoded_words);
  data_to_char += Collected[2] << (3 + 4 * decoded_words);
  decoded_words ++;

  //Print
  Serial.print("Hamming implemented, current packet: ");
  for (int k = 0; k <= 6; k++) {
    Serial.print(Collected[k]);
  }
  Serial.println("");

  if (decoded_words == 2) {
    if(data_to_char == int(Data))                         
      Serial.println("Correct input recieved");
    else
      Serial.println("Incorrect input recieved");
    
    Serial.print("Messege Recieved: ");
    Serial.println(char(data_to_char));
    data_to_char = 0;
    decoded_words = 0;
    Hamming_TX_Done = 0; 
    USART_flag = 0;
  }
}

void Simulate_noise_1bit() {                                                 // Simulate 1 bit of noise to see the Hamming algorithm in action!
  Serial.println("Simulate 1 bit of noise: ");
  int bit_num = 0;                                                           // <-- 0 LSB TO 6 MSB
  Collected[6 - bit_num] ^= 1;                                               // XOR with 1 to flip bit
}

void Simulate_noise_2bits() {                                                 // Simulate 2 bits of noise
  Serial.println("Simulate 2 bits of noise: ");
  int bit_num1 = 0;                                                           // <-- 0 LSB TO 6 MSB
  int bit_num2 = 1;
  Collected[6 - bit_num1] ^= 1;                                               // XOR with 1 to flip bit
  Collected[6 - bit_num2] ^= 1;                                               // XOR with 1 to flip bit
}

void Simulate_noise_3bits() {                                                 // Simulate 3 bits of noise
  Serial.println("Simulate 3 bits of noise: ");
  int bit_num1 = 0;                                                           // <-- 0 LSB TO 6 MSB
  int bit_num2 = 1;
  int bit_num3 = 2;
  Collected[6 - bit_num1] ^= 1;                                               // XOR with 1 to flip bit
  Collected[6 - bit_num2] ^= 1;                                               // XOR with 1 to flip bit
  Collected[6 - bit_num3] ^= 1;                                               // XOR with 1 to flip bit
}
