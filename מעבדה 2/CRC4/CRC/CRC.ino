#define RX_PIN 2
#define TX_PIN 3
#define RX_CLK 7
#define TX_CLK 8
#define clk_interval 1000
#define data_len 4
#define divisor 19
#define div_len 5

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

// CRC4-Encoder global variables
char ch = 'd';
int data = int(ch);
int data1 = data % 16;
int data2 = data >> 4;
unsigned int shifter_tx = 1;
unsigned int process_div_tx = divisor;
int wordcode[data_len + (div_len - 1)];
int word_cnt = 0;
int shift_len_tx = (data_len - 1) + (div_len - 1);
unsigned int remainder_tx1 = data1 << (div_len - 1);
unsigned int remainder_tx2 = data2 << (div_len - 1);
unsigned int remainder_tx;
int encoder_flag = 1;
int encoded_flag = 0;
int decoder_flag = 0;

// CRC4-Decoder global variables
unsigned int shifter_rx = 1;
unsigned int process_div_rx = divisor;
int shift_len_rx = (data_len - 1) + (div_len - 1);
unsigned int remainder_rx = 0;
int data_rx;
int dec_words = 0;
int fullchar;

int noise_done = 0;


void setup() {
  // Config Block
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
  pinMode(TX_CLK, OUTPUT);
  pinMode(RX_CLK, INPUT);
  Serial.begin(9600);
  remainder_tx = remainder_tx1;

}
void loop() {
  // Pre-Encoding:
  if (encoder_flag == 1) {
    noise_done = 0;
    CRC_tx();
  }

  if (encoded_flag == 1) {
    usart_tx();
    usart_rx();
  }
  if (decoder_flag == 1) {
    if (noise_done == 0) {
      noise_done = 1;
      //Simulate_noise_1bit();
      //Simulate_noise_2bits();
      //Simulate_noise_3bits();
    }
    remainder_rx = recieved << (div_len - 1);
    CRC_rx();
  }
}

void usart_tx() {
  // Transmitter Block:
  curr_time = millis();                                          // Current time sampling
  if (curr_time - ref_time == clk_interval / 2) {                // Lower half clock interval - set clock on "Low"
    digitalWrite(TX_CLK, 0);
  }
  if (curr_time - ref_time == clk_interval) {                    // Higher half clock interval - set clock on "High"
    ref_time = curr_time;
    send_bit = wordcode[(data_len - 1) + (div_len - 1) - T_bit_cnt];
    digitalWrite(TX_PIN, send_bit);
    digitalWrite(TX_CLK, 1);
    Serial.print("TX - Send bit #");
    Serial.print(T_bit_cnt);
    Serial.print(" | Value: ");
    Serial.println(send_bit);
    if (T_bit_cnt == (data_len - 1) + (div_len - 1)) {
      T_bit_cnt = 0;
      data = data >> data_len;
      remainder_tx = data << (div_len - 1);
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

    if (R_bit_cnt == data_len + (div_len - 1) ) {                // current session end-condition.
      Serial.print("Word recieved: ");
      Serial.println(recieved, BIN);
      data_rx = (recieved >> (div_len - 1));
      shift = 1;
      recieved = 0;
      R_bit_cnt = 0;
      encoded_flag = 0;
      decoder_flag = 1;
    }
  }
  last_clk = curr_clk;
}

void CRC_tx() {
  shifter_tx = 1 << (shift_len_tx);
  if ((shifter_tx & remainder_tx) == shifter_tx) {                          // Looking for leads '1'
    Serial.println(remainder_tx);
    if ((1 << (div_len - 1)) > remainder_tx) {
      for (int i = (data_len - 1) + (div_len - 1) ; i >= 0; i--) {
        if (i > (data_len - 1)) {
          wordcode[i] = remainder_tx % 2;
          remainder_tx /= 2;
        }
        else {
          if (word_cnt == 0) {
            wordcode[i] = data1 % 2;
            data1 /= 2;
          }
          else {
            wordcode[i] = data2 % 2;
            data2 /= 2;
          }
        }
      }
      encoded_flag = 1;
      encoder_flag = 0;
      if(word_cnt == 0)
        remainder_tx = remainder_tx2;
      else
        remainder_tx = remainder_tx1;
      shift_len_tx =  (data_len - 1) + (div_len - 1);
      word_cnt = 1- word_cnt;
    }
    else {
      process_div_tx = divisor << (shift_len_tx - (div_len - 1));
      remainder_tx = remainder_tx ^ process_div_tx;
    }
  }
  else
    shift_len_tx--;
}

void CRC_rx() {
  shifter_rx = 1 << (shift_len_rx);
  if ((shifter_rx & remainder_rx) == shifter_rx) {                          // Looking for leads '1'
    if ((1 << (div_len - 1)) > remainder_rx) {
      if (remainder_rx == 0) {
        Serial.print("Cyclic Redundancy Check-4 Passed");
        dec_words ++;
        fullchar += data_rx << (4*(dec_words-1)); 
        if (dec_words == 2) {
          Serial.print("Full Data Recieved: ");
          Serial.println(char(fullchar));
        }
      }
      else {
        Serial.println("Cyclic Redundancy Check-4 Failed, Data recieved incorrectly.");
      }
      recieved = 0;
      encoder_flag = 1;
      decoder_flag = 0;
    }
    else {
      process_div_rx = divisor << (shift_len_rx - (div_len - 1));
      remainder_rx = remainder_rx ^ process_div_rx;
    }
  }
  else {
    shifter_rx = shifter_rx >> 1;
    shift_len_rx--;
  }
}


void Simulate_noise_1bit() {                                                 // Simulate 1 bit of noise to see the Hamming algorithm in action!
  Serial.println("Simulate 1 bit of noise: ");
  int bit_num = 0;                                                           // <-- 0 LSB TO 6 MSB
  int flip_bit_shift = 1 << (bit_num);
  recieved = recieved ^ flip_bit_shift;                                      // XOR with 1 to flip bit
}

void Simulate_noise_2bits() {                                                 // Simulate 2 bits of noise
  Serial.println("Simulate 2 bits of noise: ");
  int bit_num1 = 0;                                                           // <-- 0 LSB TO 6 MSB
  int bit_num2 = 1;
  int flip_bit_shift = 1 << (bit_num1);
  recieved = recieved ^ flip_bit_shift;                                      // XOR with 1 to flip bit
  flip_bit_shift = 1 << (bit_num2);
  recieved = recieved ^ flip_bit_shift;                                      // XOR with 1 to flip bit
}

void Simulate_noise_3bits() {                                                 // Simulate 3 bits of noise
  Serial.println("Simulate 3 bits of noise: ");
  int bit_num1 = 0;                                                           // <-- 0 LSB TO 6 MSB
  int bit_num2 = 1;
  int bit_num3 = 2;
  int flip_bit_shift = 1 << (bit_num1);
  recieved = recieved ^ flip_bit_shift;                                      // XOR with 1 to flip bit
  flip_bit_shift = 1 << (bit_num2);
  recieved = recieved ^ flip_bit_shift;                                      // XOR with 1 to flip bit
  flip_bit_shift = 1 << (bit_num3);
  recieved = recieved ^ flip_bit_shift;                                      // XOR with 1 to flip bit
}
