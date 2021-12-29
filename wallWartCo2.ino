#include "esp_timer.h"
#include <Wire.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "Sensirion_GadgetBle_Lib.h"
const int16_t SCD_ADDRESS = 0x62;
static int64_t lastMmntTime = 0;
static int startCheckingAfterUs = 1900000;

GadgetBle gadgetBle = GadgetBle(GadgetBle::DataType::T_RH_CO2_ALT);

// Display related
#define SENSIRION_GREEN 0x6E66
#define sw_version "v1.0"

#define GFXFF 1
#define FF99  &SensirionSimple25pt7b
#define FF90  &ArchivoNarrow_Regular10pt7b
#define FF95  &ArchivoNarrow_Regular50pt7b
bool tell = 0;
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke library, pins defined in User_Setup.h

void displayInit() {
  tft.init();
  tft.setRotation(1);
}



void displayCo2(uint16_t co2) {
  if (co2 > 9999) {
    co2 = 9999;
  }

  tft.fillScreen(TFT_BLACK);

  uint8_t defaultDatum = tft.getTextDatum();

  tft.setTextSize(1);
  tft.setFreeFont(FF90);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.setTextDatum(6); // bottom left
  tft.drawString("CO2", 10, 125);

  tft.setTextDatum(8); // bottom right
  tft.drawString(gadgetBle.getDeviceIdString(), 230, 125);

  // Draw CO2 number
  if (co2 >= 1600 ) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tell = 1;
  } else if (co2 >= 1000 ) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    tell = 0;
  } else {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tell = 0;
  }

  tft.setTextDatum(8); // bottom right
  tft.setTextSize(1);
  tft.setFreeFont(FF95);
  tft.drawString(String(co2), 195, 105);

  // Draw CO2 unit
  tft.setTextSize(1);
  tft.setFreeFont(FF90);
  tft.drawString("ppm", 230, 90);

  // Revert datum setting
  tft.setTextDatum(defaultDatum);
}

void setup() {
  Serial.begin(115200);
  delay(100);
 while(!Serial);

  // output format
  Serial.println("CO2(ppm)\tTemperature(degC)\tRelativeHumidity(percent)");
  
  // Initialize the GadgetBle Library
  gadgetBle.begin();
  Serial.print("Sensirion GadgetBle Lib initialized with deviceId = ");
  Serial.println(gadgetBle.getDeviceIdString());
  
  // Initialize the SCD30 driver
  Wire.begin();
  
  displayInit();
  
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0x21);
  Wire.write(0xb1);
  Wire.endTransmission();
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  // Enjoy the splash screen for 2 seconds
  delay(500);
 
}

void loop() {
  

  if (esp_timer_get_time() - lastMmntTime >= startCheckingAfterUs) {

   
  float co2, temperature, humidity;
  uint8_t data[12], counter;

  // send read data command
  Wire.beginTransmission(SCD_ADDRESS);
  Wire.write(0xec);
  Wire.write(0x05);
  Wire.endTransmission();Wire.requestFrom(SCD_ADDRESS, 12);
  counter = 0;
  while (Wire.available()) {
    data[counter++] = Wire.read();
  }
  
  // floating point conversion according to datasheet
  co2 = (float)((uint16_t)data[0] << 8 | data[1]);
  // convert T in degC
  temperature = -45 + 175 * (float)((uint16_t)data[3] << 8 | data[4]) / 65536;
  // convert RH in %
  humidity = 100 * (float)((uint16_t)data[6] << 8 | data[7]) / 65536;

  Serial.print(co2);
  Serial.print("\t");
  Serial.print(temperature);
  Serial.print("\t");
  Serial.print(humidity);
  Serial.println();
      gadgetBle.writeCO2(co2);
      gadgetBle.writeTemperature(temperature);
      gadgetBle.writeHumidity(humidity);

      gadgetBle.commit();
      lastMmntTime = esp_timer_get_time();
      displayCo2((uint16_t) std::round(co2));
    if (tell){
  digitalWrite(13,HIGH);
  delay(1000);
  digitalWrite(13, LOW);
  delay(1000);
    }
  }

  gadgetBle.handleEvents();
  delay(3);
 
}
