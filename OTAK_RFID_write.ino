#include <SPI.h>
#include <MFRC522.h>

/*Using Hardware SPI of Arduino */
/*MOSI (11), MISO (12) and SCK (13) are fixed */
/*You can configure SS and RST Pins*/
#define SS_PIN D2  /* Slave Select Pin */
#define RST_PIN D1  /* Reset Pin */

/* Create an instance of MFRC522 */
MFRC522 mfrc522(SS_PIN, RST_PIN);
/* Create an instance of MIFARE_Key */
MFRC522::MIFARE_Key key;          

/* Set the block to which we want to write data */
/* Be aware of Sector Trailer Blocks */
int blockNum_id = 4;
int blockNum_date = 5;
int blockNum_type = 6;
byte bufferLen = 18;
String data;
String id_txt = "ID unit:";
String date_txt = ", Date:";
String type_txt = ", Product type:";
String id_write = "ID  :";
String date_write = "D:";
String type_write = "Product type:";

bool ready_data = false;

MFRC522::StatusCode status;

char* extractSubstring(String original, int startIndex, int endIndex) {
  int length = endIndex - startIndex + 1;
  char* result = new char[length + 1];
  char temp[original.length() + 1];
  original.toCharArray(temp, sizeof(temp));
  strncpy(result, temp + startIndex, length);
  result[length] = '\0';
  return result;
}

void setup() 
{
  /* Initialize serial communications with the PC */
  Serial.begin(9600);
  /* Initialize SPI bus */
  SPI.begin();
  /* Initialize MFRC522 Module */
  mfrc522.PCD_Init();
  Serial.println("Scan a MIFARE 1K Tag to write data...");
}

void loop()
{
  if (Serial.available() > 0) {  // Check if data is available to read 
    data = Serial.readString();  // Read the data from serial port 
    Serial.print("Received data: "); 
    Serial.println(data);  // Print the received data 
    ready_data = true;
  }
  else {
    if (ready_data == false) return;
  }
  /* Prepare the ksy for authentication */
  /* All keys are set to FFFFFFFFFFFFh at chip delivery from the factory */
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  /* Look for new cards */
  /* Reset the loop if no new card is present on RC522 Reader */
  if ( ! mfrc522.PICC_IsNewCardPresent())
  {
    return;
  }
  
  /* Select one of the cards */
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  Serial.print("\n");
  Serial.println("**Card Detected**");

  int id_idx = data.indexOf(id_txt);
  int date_idx = data.indexOf(date_txt);
  int type_idx = data.indexOf(type_txt);

  String idString = id_write += data.substring(id_idx+id_txt.length(), date_idx);
  String dateString = date_write += data.substring(date_idx+date_txt.length(), type_idx);
  String typeString = type_write += data.substring(type_idx+type_txt.length());

  // Convert these strings to byte arrays
  byte blockData_id[16] = {}; // Initialize with zeroes
  byte blockData_date[16] = {}; // Initialize with zeroes
  byte blockData_type[16] = {}; // Initialize with zeroes

  // Copy data from the extracted strings to the byte arrays
  idString.getBytes(blockData_id, sizeof(blockData_id));  // Convert to bytes
  dateString.getBytes(blockData_date, sizeof(blockData_date));  // Convert to bytes
  typeString.getBytes(blockData_type, sizeof(blockData_type));

  byte readBlockData_id[18];
  byte readBlockData_date[18];
  byte readBlockData_type[18];

   /* Call 'WriteDataToBlock' function, which will write data to the block */
   Serial.print("\n");
   Serial.println("Writing to Data Block...");
   WriteDataToBlock(blockNum_id, blockData_id);
   WriteDataToBlock(blockNum_date, blockData_date);
   WriteDataToBlock(blockNum_type, blockData_type);
   
   /* Read data from the same block */
   Serial.print("\n");
   Serial.println("Reading from Data Block...");
   ReadDataFromBlock(blockNum_id, readBlockData_id);
   ReadDataFromBlock(blockNum_date, readBlockData_date);
   ReadDataFromBlock(blockNum_type, readBlockData_type);
   
   /* Print the data read from block */
   Serial.print("\n");
   Serial.print("Data in Block ID:");
   Serial.print(blockNum_id);
   Serial.print(" --> ");
   for (int j=0 ; j<16 ; j++)
   {
     Serial.write(readBlockData_id[j]);
   }
   Serial.print("\n");

   Serial.print("\n");
   Serial.print("Data in Block Date:");
   Serial.print(blockNum_date);
   Serial.print(" --> ");
   for (int j=0 ; j<16 ; j++)
   {
     Serial.write(readBlockData_date[j]);
   }
   Serial.print("\n");

   Serial.print("\n");
   Serial.print("Data in Block Type:");
   Serial.print(blockNum_type);
   Serial.print(" --> ");
   for (int j=0 ; j<16 ; j++)
   {
     Serial.write(readBlockData_type[j]);
   }
   Serial.print("\n");
   
   mfrc522.PICC_HaltA();
   mfrc522.PCD_StopCrypto1();
   ready_data = false;
   data = "";
}


void WriteDataToBlock(int blockNum, byte blockData[]) 
{
  /* Authenticating the desired data block for write access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Authentication failed for Write: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else
  {
    Serial.println("Authentication success");
  }

  
  /* Write data to the block */
  status = mfrc522.MIFARE_Write(blockNum, blockData, 16);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Writing to Block failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else
  {
    Serial.println("Data was written into Block successfully");
  }
  
}

void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{
  /* Authenticating the desired data block for Read access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK)
  {
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  }
  else
  {
    Serial.println("Authentication success");
  }

  /* Reading data from the Block */
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  else
  {
    Serial.println("Block was read successfully");  
  }

}
