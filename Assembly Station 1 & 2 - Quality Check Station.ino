#include <SPI.h>
#include <MFRC522.h>

/* Using Hardware SPI of Arduino */
/* MOSI (11), MISO (12) and SCK (13) are fixed */
/* You can configure SS and RST Pins */
#define SS_PIN D3  /* Slave Select Pin */
#define RST_PIN D4  /* Reset Pin */

/* Create an instance of MFRC522 */
MFRC522 mfrc522(SS_PIN, RST_PIN);
/* Create an instance of MIFARE_Key */
MFRC522::MIFARE_Key key;          

/* Set the block to which we want to read data */
/* Be aware of Sector Trailer Blocks */
int blockNum_id = 4;
int blockNum_date = 5;
int blockNum_type = 6;  

/* Create another array to read data from Block */
/* Length of buffer should be 2 Bytes more than the size of Block (16 Bytes) */
byte bufferLen = 18;
byte readBlockData_id[18];
byte readBlockData_date[18];
byte readBlockData_type[18];

MFRC522::StatusCode status;

void setup() 
{
  /* Initialize serial communications with the PC */
  Serial.begin(9600);
  /* Initialize SPI bus */
  SPI.begin();
  /* Initialize MFRC522 Module */
  mfrc522.PCD_Init();
  //Serial.println("Scan a MIFARE 1K Tag to read data...");
}

void loop()
{
  /* Prepare the key for authentication */
  /* All keys are set to FFFFFFFFFFFFh at chip delivery from the factory */
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  
  /* Look for new cards */
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) 
  {
    //Serial.print("\n");
    //Serial.println("**Card Detected**");
    
    /* Print UID of the Card */
    //Serial.print(F("Card UID:"));
    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
      //Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      //Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    //Serial.print("\n");

    /* Read data from the blocks */
    ReadDataFromBlock(blockNum_id, readBlockData_id);
    ReadDataFromBlock(blockNum_date, readBlockData_date);
    ReadDataFromBlock(blockNum_type, readBlockData_type);

    /* Construct a string to print the data */
    String value = "Station_ID: 1,";

    for (uint8_t i = 0; i < 16; i++)
    {
      if (byte(readBlockData_id[i]) != 0x00) value += (char)readBlockData_id[i];
    }
    value.trim();
    value += ",";

    for (uint8_t i = 0; i < 16; i++)
    {
      if (byte(readBlockData_date[i]) != 0x00) value += (char)readBlockData_date[i];
    }
    value.trim();
    value += ",";

    for (uint8_t i = 0; i < 16; i++)
    {
      if (byte(readBlockData_type[i]) != 0x00) value += (char)readBlockData_type[i];
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();

    }

    Serial.println(value);
  }

  /* Delay for 3 seconds before reading again */
  delay(100);
}

void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{
  /* Authenticating the desired data block for Read access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK)
  {
     Serial.print("error");
     //Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  }
  else
  {
    //Serial.println("Authentication success");
  }

  /* Reading data from the Block */
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("error");
    //Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else
  {
    //Serial.println("Block was read successfully");  
  }
}
