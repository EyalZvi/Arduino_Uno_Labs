#ifndef EthernetLab
#define EthernetLab

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#define PRES 0
#define NON_PRES 1

char remote_IP[] = "132.72.110.30";
int remote_Port, statusLine = 2, csmaMode, persistent;
unsigned long newTimePackage, oldTimePackage, delaystatus;
unsigned long newTimeSlot, oldTimeSlot;
unsigned long prog = 300;
EthernetUDP Udp;

void setAddress(byte number, byte pair){
  int localip;
  // Local Arduino settings:
  unsigned int localPort = (int)pair + (int)number + 10000;
  byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, number, pair };
  remote_Port = 4444;

  // Set settings, DHCP
  Ethernet.begin(mac);
  Udp.begin(localPort);
}

void startPackage(void * payload, int payload_size){
    int rantime;
  if (statusLine == 2){
      int random_number = random(0,100);
      if (csmaMode == PRES && persistent == 1){
          rantime = 40;
          persistent = 0;
      }
      else {
          rantime = 10;
      }
      if (random_number < rantime){
          statusLine = 0;
          newTimePackage = micros();
          oldTimePackage = newTimePackage;
          delaystatus = random(90, 100);
      }
      else{
          Udp.beginPacket(remote_IP, remote_Port);
          Udp.write((char *)payload, payload_size);
          statusLine = 1;
      }
  }
  else
      Serial.println("WARRNING: Don't forget to call endPackage");
}

int checkLine(){
  switch (statusLine){
      case 0:
        newTimePackage = millis();
        if (newTimePackage - oldTimePackage >= delaystatus){
            delaystatus = 0;
            return statusLine;
        }
        return 1;
        break;
      case 1:
        return statusLine;
        break;
      case 2:
          int random_number = random(0,100);
          if (random_number < 60){
              if (csmaMode == PRES)
                  persistent = 1;
              return 0;
          }
          return 1;
          break;
  }
}

int endPackage(int option){
  int statusPacket;
  switch (option){
      case 0:
          Ethernet.maintain();
          statusPacket = 0;
          break;
      case 1:
          if (statusLine != 2){
            Udp.endPacket();
          }
          else {
            Serial.println("WARRNING: Call startPackage first");
            return 0;
          }
          statusPacket = 1;
          break;
      default:
          Serial.println("WARRNING: Wrong status number");
  }
  statusLine = 2;
  return statusPacket;
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

void setMode(int mod){
    csmaMode = mod;
}


#endif /* EthernetLab */
