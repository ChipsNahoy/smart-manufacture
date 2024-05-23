#include <TM1637Display.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPI.h>

// 1: Bolt
// 2: Nut silver
// 3: Nut black
// 4: Washer silver
// 5: Washer black

#define ledPin 2

// Display
#define CLK1 32
#define CLK2 33
#define CLK3 25
#define DIO1 21
#define DIO2 17
#define DIO3 16

// RFID
#define RST_PIN         4
#define SS_1_PIN        13
#define SS_2_PIN        12
#define NR_OF_READERS   2
byte ssPins[] = {SS_1_PIN, SS_2_PIN};
MFRC522 mfrc522[NR_OF_READERS];

// SS1 = ingoing, SS2 = outgoing

MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

// Wi-Fi
const char *ssid = "AndroidAPCA5E";
const char *password = "Apahayo.";
WiFiClient wifiClient;

// MQTT
const char *mqtt_server = "192.168.132.184";
const char *storage_topic = "storage";
const char* mqtt_username = "storage_esp";
const char* mqtt_password = "wololo";
const char *clientID = "Storage_Station_1";
PubSubClient client(mqtt_server, 1885, wifiClient);

int block_id = 4;
int block_date = 5;
int block_type = 6;

int inventoryX = 0;
int inventoryY = 0;
int inventoryZ = 0;

TM1637Display displayX(CLK1, DIO1);
TM1637Display displayY(CLK2, DIO2);
TM1637Display displayZ(CLK3, DIO3);

void refresh_lcd(TM1637Display display, int quantity)
{
  display.setBrightness(0x0f);
  display.clear();
  // Show decimal numbers with/without leading zeros
  display.showNumberDec(quantity, false);

}

void all_lcd()
{
  refresh_lcd(displayX, inventoryX);
  refresh_lcd(displayY, inventoryY);
  refresh_lcd(displayZ, inventoryZ);
}

int err_check(byte buffer_id[], byte buffer_date[], byte buffer_type[], byte &len, MFRC522 rfid)
{
  int cond = 0;

  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block_id, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(rfid.GetStatusCodeName(status));
    cond += 1;
    return cond;
  }

  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block_date, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(rfid.GetStatusCodeName(status));
    cond += 1;
    return cond;
  }

  status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, block_type, &key, &(rfid.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed for Read: ");
    Serial.println(rfid.GetStatusCodeName(status));
    cond += 1;
    return cond;
  }
 
  status = rfid.MIFARE_Read(block_id, buffer_id, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(rfid.GetStatusCodeName(status));
    cond += 1;
    return cond;
  }

  status = rfid.MIFARE_Read(block_date, buffer_date, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(rfid.GetStatusCodeName(status));
    cond += 1;
    return cond;
  }

  status = rfid.MIFARE_Read(block_type, buffer_type, &len);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(rfid.GetStatusCodeName(status));
    cond += 1;
    return cond;
  }

  return cond;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connect_mqtt() {
  while (! client.connect(clientID, mqtt_username, mqtt_password)) Serial.println("Connection to MQTT Broker failed…");
  Serial.println("Connected to MQTT Broker!");
}


void setup() {
  Serial.begin(9600);
  setup_wifi();
  SPI.begin();
  pinMode(ledPin, OUTPUT);
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
    mfrc522[reader].PCD_Init(ssPins[reader], RST_PIN);
  }
  all_lcd();
}

void loop() {  
  for (uint8_t reader = 0; reader < NR_OF_READERS; reader++) {
      // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
      for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
      
      if (mfrc522[reader].PICC_IsNewCardPresent() && mfrc522[reader].PICC_ReadCardSerial()) {
    
          byte len = 18;

          byte buffer_id[18];
          byte buffer_date[18];
          byte buffer_type[18];

          if (err_check(buffer_id, buffer_date, buffer_type, len, mfrc522[reader]) != 0) {
            String value = "ID: error,Date: error,Product_type: error";
            Serial.println(value);
            mfrc522[reader].PICC_HaltA();
            mfrc522[reader].PCD_StopCrypto1();
            return;
          }

          String value_data = "Station_ID: 1,";

          for (uint8_t i = 0; i < 16; i++)
          {
              if (byte(buffer_id[i]) != 0x00) value_data += (char)buffer_id[i];
          }
          value_data.trim();
          value_data += ",";


          for (uint8_t i = 0; i < 16; i++)
          {
              if (byte(buffer_date[i]) != 0x00) value_data += (char)buffer_date[i];
          }
          value_data.trim();
          value_data += ",";


          for (uint8_t i = 0; i < 16; i++)
          {
              if (byte(buffer_type[i]) != 0x00) value_data += (char)buffer_type[i];
          }
          value_data.trim();

          Serial.println("Show inventory start");
          if (reader == 0) {           
              char prod = value_data.charAt(value_data.length()-1);
              if      (prod == 'X') inventoryX += 1;
              else if (prod == 'Y') inventoryY += 1;
              else if (prod == 'Z') inventoryZ += 1;
              value_data += ",in";
          }
          else if (reader == 1) {
              char prod = value_data.charAt(value_data.length()-1);
              if      (prod == 'X' && inventoryX > 0) inventoryX -= 1;
              else if (prod == 'Y' && inventoryY > 0) inventoryY -= 1;
              else if (prod == 'Z' && inventoryZ > 0) inventoryZ -= 1;
              else {
                Serial.println("Empty inventory, no inventory can be pulled!");
                mfrc522[reader].PICC_HaltA();
                mfrc522[reader].PCD_StopCrypto1();
                return;
              }
              value_data += ",out";
          }
          Serial.println("Update inventory value done!");
          Serial.println(value_data);

          connect_mqtt();
          if (client.publish(storage_topic, value_data.c_str())) {
            Serial.println("Data sent!");
          }
          else {
            Serial.println("Data failed to send. Reconnecting...");
            client.connect(clientID, mqtt_username, mqtt_password);
            delay(10); // This delay ensures that client.publish doesn’t clash with the client.connect call
            client.publish(storage_topic, value_data.c_str());
          }
          
          all_lcd();
          mfrc522[reader].PICC_HaltA();
          mfrc522[reader].PCD_StopCrypto1();
          digitalWrite(ledPin, HIGH);
          delay(500);
          digitalWrite(ledPin, LOW);
      }
  }
}