#ifndef EthernetLab
#define EthernetLab
#define ErrorBitProb 10
#if defined(PROGMEM)
    #define FLASH_PROGMEM PROGMEM
    #define FLASH_READ_DWORD(x) (pgm_read_dword_near(x))
#else
    #define FLASH_PROGMEM
    #define FLASH_READ_DWORD(x) (*(uint32_t*)(x))
#endif

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

char remote_IP[16];
int remote_Port;
unsigned long PropagationDelay = 1000;

static const uint32_t crc32_table[] FLASH_PROGMEM = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
};

EthernetUDP Udp;

class CRC32
{
public:
    CRC32();
    void reset();
    void update(const uint8_t& data);
    
    template <typename Type>
    void update(const Type& data)
    {
        update(&data, 1);
    }
    
    template <typename Type>
    void update(const Type* data, size_t size)
    {
        size_t nBytes = size * sizeof(Type);
        const uint8_t* pData = (const uint8_t*)data;

        for (size_t i = 0; i < nBytes; i++)
        {
            update(pData[i]);
        }
    }
    
    uint32_t finalize() const;

    template <typename Type>
    static uint32_t calculate(const Type* data, size_t size)
    {
        CRC32 crc;
        crc.update(data, size);
        return crc.finalize();
    }

private:
    uint32_t _state = ~0L;

};

CRC32::CRC32()
{
    reset();
}


void CRC32::reset()
{
    _state = ~0L;
}


void CRC32::update(const uint8_t& data)
{
    // via http://forum.arduino.cc/index.php?topic=91179.0
    uint8_t tbl_idx = 0;

    tbl_idx = _state ^ (data >> (0 * 4));
    _state = FLASH_READ_DWORD(crc32_table + (tbl_idx & 0x0f)) ^ (_state >> 4);
    tbl_idx = _state ^ (data >> (1 * 4));
    _state = FLASH_READ_DWORD(crc32_table + (tbl_idx & 0x0f)) ^ (_state >> 4);
}


uint32_t CRC32::finalize() const
{
    return ~_state;
}

uint32_t calculateCRC(const char * payload, size_t payload_size){
    uint32_t checksum = CRC32::calculate(payload, payload_size);
    return checksum;
}

void setAddress(int number, int pair){
  int remote = (1-number)%2;
  int localip;
  // Local Arduino settings:
  unsigned int localPort = pair + number + 10000;
  byte mac[] = {0x11,0x11,0x11, (byte)number, (byte)pair};
  
  if (number){
    localip = pair;
    sprintf(remote_IP, "111.111.111.%d",255 - pair);
  }else{
    localip = 255 - pair;
    sprintf(remote_IP, "111.111.111.%d",pair);
  }
  IPAddress ip(111,111,111,localip);
  
  remote_Port = pair + remote + 10000;

  // Set settings
  Ethernet.begin(mac, ip);
  Udp.begin(localPort);
}

int sendPackage(void * payload,int payload_size){
  // The functions return 0 if the line is busy, else return 1.
  static unsigned long oldTime=0;
  static unsigned long newTime=0;
  newTime = millis();
  if((newTime-oldTime)>PropagationDelay){
    Udp.beginPacket(remote_IP, remote_Port);
    int random_number = random(0,101);
    if (random_number < 10)
      *((char*)payload+3) ^= 1;
    Udp.write((char *)payload, payload_size);
    Udp.endPacket();
    oldTime =newTime;
    return 1;
  }
  else return 0;
}

int readPackage(void * payload, int payload_size){
  // The functions return 0 if no new payload, else return 1.
  int packetSize = Udp.parsePacket();
  if (packetSize){
      Udp.read((char *)payload, payload_size);
      return 1;  
  }
  return 0;
}

void setDelay(unsigned long new_Delay){
    PropagationDelay = new_Delay;
}


#endif /* EthernetLab */
