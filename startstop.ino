#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define GPIO_PWM0A_OUT 19   //HV
#define GPIO_PWM1A_OUT 18   //HV

int freq0 = 50000;
float duty0 = 0;
float duty1 = 0;
bool firstButton = 1;

void setuppwm(){
   mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT);
   mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, GPIO_PWM1A_OUT);
 mcpwm_config_t pwm_config0;
     pwm_config0.frequency = freq0;  //frequency 
     pwm_config0.cmpr_a = 0;      //duty cycle of PWMxA = 50.0%
     pwm_config0.cmpr_b = 0;      //duty cycle of PWMxB = 50.0%
     pwm_config0.counter_mode = MCPWM_UP_COUNTER; // Up-down counter (triangle wave)
     pwm_config0.duty_mode = MCPWM_DUTY_MODE_0; // Active HIGH
   mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config0);    //Configure PWM0A & PWM0B with above settings
   mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &pwm_config0);    //Configure PWM0A & PWM0B with above settings
   delay(20);
   mcpwm_set_frequency(MCPWM_UNIT_1, MCPWM_TIMER_1,freq0);
   mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty0);
   mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, duty1);
   
 }
void setup1() {
 setuppwm();
}

//void loop(){
//  
//}

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint8_t txValue = 0;
//unsigned char leftMotorSpeed = 75; // CHANGED to duty1
//unsigned char rightMotorSpeed = 75; // CHANGED TO duty0
uint8_t bnum;
boolean pressed;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        
        if (rxValue[1] == 'B') {
          bnum = rxValue[2] - '0'; // button number: UP is 5, RIGHT is 8, DOWN is 6, LEFT is 7
          pressed = rxValue[3] - '0'; // pressed will be 1 if button press and 0 if release
          // Serial.print ("Button "); Serial.print(bnum);


          if (pressed) {
            Serial.println(" pressed");
          } else {
            Serial.println(" released");
          }
        }
        Serial.println();
      }
    }
};


void setup() {
  setup1();
  Serial.begin(115200);
  
  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
										CHARACTERISTIC_UUID_TX,
										BLECharacteristic::PROPERTY_NOTIFY
									);
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
											 CHARACTERISTIC_UUID_RX,
											BLECharacteristic::PROPERTY_WRITE
										);

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {

  if (deviceConnected) {
      pTxCharacteristic->setValue(&txValue, 1);
      pTxCharacteristic->notify();
      txValue++;
  delay(10); // bluetooth stack will go into congestion, if too many packets are sent
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
  // do stuff here on connecting
      oldDeviceConnected = deviceConnected;
  }

  if (bnum == 5 && pressed) // UP button pressed
  {
    int count = 0;
    while(!(bnum == 5 && !pressed)) // while UP button NOT released
    {
      if(firstButton)
      {
        duty1 = 75;
        duty0 = 75;
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, duty1);
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty0);
        firstButton = 0;
      }
      else{
        if(count >= 40)
        {
          if(duty1 < 100)
          {
            duty1++;
          }
          if(duty0 < 100)
          {
            duty0++;
          }
          mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, duty1);
          mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty0);
          count = 0;
        }
        delay(2);
        count++;
        Serial.print("Here1: ");
        Serial.print(duty0);
        Serial.print(", ");
        Serial.println(duty1);
      }
    }
  }
  if (bnum == 6 && pressed) // DOWN button pressed
  {
    int count = 0;
    while(!(bnum == 6 && !pressed)) // while DOWN button NOT released
    {
      if(firstButton)
      {
        duty1 = 75;
        duty0 = 75;
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, duty1);
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty0);
        firstButton = 0;
      }
      else
      {
        if(count >= 40)
        {
          if(duty1 > 50)
          {
            duty1--;
          }
          if(duty0 > 50)
          {
            duty0--;
          }
          mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, duty1);
          mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty0);
          count = 0;
        }
        delay(2);
        count++;
        Serial.print("Here5: ");
        Serial.print(duty0);
        Serial.print(", ");
        Serial.println(duty1);
      }
    }
  }
  if (bnum == 7 && pressed) // LEFT button pressed
  {
    int count = 0;
    int initialLeftSpeed = duty1;
    while(!(bnum == 7 && !pressed)) // while LEFT button NOT released
    {
      if(firstButton)
      {
        duty1 = 75;
        duty0 = 75;
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, duty1);
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty0);
        firstButton = 0;
      }
      else{
        if(count >= 40)
        {
          if(duty1 > (initialLeftSpeed-20) && duty1 > 50)
          {
            duty1--;
          }
          mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, duty1);
          count = 0;
        }
        delay(2);
        count++;
        Serial.print("Here4: ");
        Serial.print(duty0);
        Serial.print(", ");
        Serial.println(duty1);
      }
    }
    while (duty1 != initialLeftSpeed)
    {
      if(count >= 40)
      {
        duty1++;
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, duty1);
        count = 0;
      }
      delay(2);
      count++;
        Serial.print("Here0: ");
        Serial.print(duty0);
        Serial.print(", ");
        Serial.println(duty1);
    }
  }


  if (bnum == 8 && pressed) // RIGHT button pressed
  {
    int count = 0;
    int initialRightSpeed = duty0;
    while(!(bnum == 8 && !pressed)) // while RIGHT button NOT released
    {
      if(firstButton)
      {
        duty1 = 75;
        duty0 = 75;
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, duty1);
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty0);
        firstButton = 0;
      }
      else
      {
        if(count >= 40)
        {
          if(duty0 > (initialRightSpeed-20) && duty0 > 50)
          {
            duty0--;
          }
          mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty0);
          count = 0;
        }
        delay(2);
        count++;
        Serial.print("Here3: ");
        Serial.print(duty0);
        Serial.print(", ");
        Serial.println(duty1);
      }
    }
    while (duty0 != initialRightSpeed)
    {
      if(count >= 40)
      {
        duty0++;
        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty0);
        count = 0;
      }
      delay(2);
      count++;
        Serial.print("Here2: ");
        Serial.print(duty0);
        Serial.print(", ");
        Serial.println(duty1);
    }
  }
}
