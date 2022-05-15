#define RX_PIN 2
#define TX_PIN 3
#define RX_CLK 7
#define TX_CLK 8
#define clk_interval 1000
#define data_len 8

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


void setup() {

  // Config Block
  pinMode(TX_PIN, OUTPUT);
  pinMode(RX_PIN, INPUT);
  pinMode(TX_CLK, OUTPUT);
  pinMode(RX_CLK, INPUT);
  Serial.begin(9600);

}

void loop() {
  // Priority Block:
  usart_tx();
  usart_rx();
}
void usart_tx() {
  // Transmitter Block:
  curr_time = millis();                                          // Current time sampling
  if(curr_time - ref_time == clk_interval/2){                    // Lower half clock interval - set clock on "Low"
    digitalWrite(TX_CLK,0);
  } 
  if(curr_time - ref_time == clk_interval){                      // Higher half clock interval - set clock on "High"
    ref_time = curr_time;
    send_bit = random(0,2);
    digitalWrite(TX_PIN, send_bit);
    digitalWrite(TX_CLK,1);
    Serial.print("TX - Send bit #");
    Serial.print(T_bit_cnt);
    Serial.print(" | Value: ");
    Serial.println(send_bit);
    if(T_bit_cnt == data_len - 1)
      T_bit_cnt = 0;
    else
      T_bit_cnt ++;
  }
}

void usart_rx() {
  curr_clk = digitalRead(RX_CLK);
  if(curr_clk == 0 && last_clk == 1){                            // Detecting 'Falling Edge'
    curr_bit = digitalRead(RX_PIN);
    recieved += curr_bit * shift;
    shift = shift << 1;
    R_bit_cnt ++;
    
    if(R_bit_cnt == data_len){                                          // current session end-condition.
        Serial.print("Word recieved: ");
        Serial.println(recieved, BIN);
        
        shift = 1;
        recieved = 0;
        R_bit_cnt = 0;
    }
   }
     last_clk = curr_clk;
 }
