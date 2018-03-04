// LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#define LCD_ADDR 0x27
#define LCD_COLS 16
#define LCD_ROWS 2
LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);

// RFID
#include <SPI.h>
#include <MFRC522.h>
#define RST_PIN         D3         // Configurable, see typical pin layout above
#define SS_PIN          D8        // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

// WiFi
#include <ESP8266WiFi.h>
const char* ssid = "Bia&CAIO!";
const char* password = "pipoca71";
WiFiClient nodeMCUWiFi;

// MQTT
#include <PubSubClient.h>
#define MQTT_PORT 1883
PubSubClient client(nodeMCUWiFi);
const char* mqtt_Broker = "iot.eclipse.org";
const char* mqtt_ClientId = "hospitalCheckin";
const String mqtt_topic = "fiap/29aso/paciente";

struct pasciente {
  String nome = "";
  String sobrenome = "";
  String convenio = "";
  String cartao = "";
};

void setup() {
  Serial.begin(115200);
  // Init LCD
  initLcd();
  initRFID();
  conectarWifi();
  client.setServer(mqtt_Broker, MQTT_PORT);

  displayMessage("Projeto IOT FIAP", "SEJA BEM VINDO!");
  delay(5000);
  displayMessage(" Aproxime o seu", "cartao do leitor");
}

void initLcd() {
  lcd.init();
  lcd.begin(16, 2);
  lcd.backlight();
}

void initRFID() {
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522

  //Prepara chave - padrao de fabrica = FFFFFFFFFFFFh -- RFID Key A
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
}

void loop() {
  modo_leitura();
}

void displayMessage(String m1, String m2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(m1);
  lcd.setCursor(0, 1);
  lcd.print(m2);
}

void conectarWifi() {
  delay(10);
  Serial.println("WiFi!!");
  Serial.println("Conectando!");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void publicarCheckIn(pasciente p) {
  if (!client.connected()) {
    reconnectMqtt();
  }
  p.nome.trim();
  p.sobrenome.trim();
  p.convenio.trim();
  p.cartao.trim();
  String topic = mqtt_topic;
  topic.concat("/");
  topic.concat(p.nome);
  topic.concat("/");
  topic.concat(p.sobrenome);
  topic.concat("/");
  topic.concat(p.convenio);
  topic.concat("/");
  topic.concat(p.cartao);
  char* newTopic;
  topic.toCharArray(newTopic, 50);
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

void reconnectMqtt() {
  while (!client.connected()) {
    client.connect(mqtt_ClientId);
  }
}

void mensagem_inicial_cartao()
{
  Serial.println("Aproxime o seu cartao do leitor...");
  lcd.clear();
  lcd.print(" Aproxime o seu");
  lcd.setCursor(0, 1);
  lcd.print("cartao do leitor");
}

void mensageminicial()
{
  Serial.println("nIniciando gravacao...");
  Serial.println();
  lcd.clear();
  lcd.print("Selecione o modo");
  lcd.setCursor(0, 1);
  lcd.print("leitura/gravacao");
}

void modo_leitura()
{
  mensagem_inicial_cartao();
  //Aguarda cartao
  while ( ! mfrc522.PICC_IsNewCardPresent())
  {
    delay(100);
  }
  if ( ! mfrc522.PICC_ReadCardSerial())
  {
    return;
  }

  pasciente p1;
  
  //Mostra UID na serial
  Serial.print("UID da tag : ");
  String conteudo = "";
  byte letra;
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();

  //Obtem os dados do setor 1, bloco 4 = Nome
  byte sector         = 1;
  byte blockAddr      = 4;
  byte trailerBlock   = 7;
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  //Autenticacao usando chave A
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  //Mostra os dados do nome no Serial Monitor e LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  for (byte i = 0; i < 16; i++)
  {
    p1.nome.concat(char(buffer[i]));
    lcd.write(char(buffer[i]));
  }
  Serial.print("Nome: ");
  Serial.println(p1.nome);
  Serial.println();

  //Obtem os dados do setor 0, bloco 1 = Sobrenome
  sector         = 0;
  blockAddr      = 1;
  trailerBlock   = 3;

  //Autenticacao usando chave A
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  //Mostra os dados do sobrenome no Serial Monitor e LCD
  lcd.setCursor(0, 1);
  for (byte i = 0; i < 16; i++)
  {
    p1.sobrenome.concat(char(buffer[i]));
    lcd.write(char(buffer[i]));
  }
  Serial.print("Sobrenome: ");
  Serial.println(p1.sobrenome);
  Serial.println();

  //Obtem os dados do setor 2, bloco 8 = Convenio
  sector         = 2;
  blockAddr      = 8;
  trailerBlock   = 11;

  //Autenticacao usando chave A
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  //Mostra os dados do Convenio no Serial Monitor e LCD
  lcd.setCursor(0, 1);
  for (byte i = 0; i < 16; i++)
  {
    p1.convenio.concat(char(buffer[i]));
    lcd.write(char(buffer[i]));
  }
  Serial.print("Convenio: ");
  Serial.println(p1.convenio);
  Serial.println();

  //Obtem os dados do setor 3, bloco 12 = Cartao
  sector         = 3;
  blockAddr      = 12;
  trailerBlock   = 15;

  //Autenticacao usando chave A
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK)
  {
    Serial.print(F("MIFARE_Read() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }
  //Mostra os dados do Cartao no Serial Monitor e LCD
  lcd.setCursor(0, 1);
  for (byte i = 0; i < 16; i++)
  {
    p1.cartao.concat(char(buffer[i]));
    lcd.write(char(buffer[i]));
  }
  Serial.print("N Cartao: ");
  Serial.println(p1.cartao);
  
  publicarCheckIn(p1);
  displayMessage(" Check In ", " Realizado!");
  delay(4000);
  displayMessage("Seja bem", " vindo(a)!");
  delay(10000);
  
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  delay(3000);
  mensageminicial();
}

void modo_gravacao()
{
  mensagem_inicial_cartao();
  //Aguarda cartao
  while ( ! mfrc522.PICC_IsNewCardPresent()) {
    delay(100);
  }
  if ( ! mfrc522.PICC_ReadCardSerial())    return;

  //Mostra UID na serial
  Serial.print(F("UID do Cartao: "));    //Dump UID
  for (byte i = 0; i < mfrc522.uid.size; i++)
  {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  //Mostra o tipo do cartao
  Serial.print(F("nTipo do PICC: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  byte buffer[34];
  byte block;
  MFRC522::StatusCode status;
  byte len;

  Serial.setTimeout(20000L) ;
  Serial.println(F("Digite o sobrenome,em seguida o caractere #"));
  lcd.clear();
  lcd.print("Digite o sobreno");
  lcd.setCursor(0, 1);
  lcd.print("me + #");
  len = Serial.readBytesUntil('#', (char *) buffer, 30) ;
  for (byte i = len; i < 30; i++) buffer[i] = ' ';

  block = 1;
  //Serial.println(F("Autenticacao usando chave A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //Grava no bloco 1
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  block = 2;
  //Serial.println(F("Autenticacao usando chave A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //Grava no bloco 2
  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  Serial.println(F("Digite o nome, em seguida o caractere #"));
  lcd.clear();
  lcd.print("Digite o nome e");
  lcd.setCursor(0, 1);
  lcd.print("em seguida #");
  len = Serial.readBytesUntil('#', (char *) buffer, 20) ;
  for (byte i = len; i < 20; i++) buffer[i] = ' ';

  block = 4;
  //Serial.println(F("Autenticacao usando chave A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //Grava no bloco 4
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  block = 5;
  //Serial.println(F("Authenticating using key A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //Grava no bloco 5
  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    //return;
  }

  Serial.println(F("Digite o nome do convenio, em seguida o caractere #"));
  lcd.clear();
  lcd.print("Digite o nome e");
  lcd.setCursor(0, 1);
  lcd.print("em seguida #");
  len = Serial.readBytesUntil('#', (char *) buffer, 20) ;
  for (byte i = len; i < 20; i++) buffer[i] = ' ';

  block = 8;
  //Serial.println(F("Autenticacao usando chave A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //Grava no bloco 8
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  block = 9;
  //Serial.println(F("Authenticating using key A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //Grava no bloco 9
  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    //return;
  }

  Serial.println(F("Digite o numero do cartao, em seguida o caractere #"));
  lcd.clear();
  lcd.print("Digite o nome e");
  lcd.setCursor(0, 1);
  lcd.print("em seguida #");
  len = Serial.readBytesUntil('#', (char *) buffer, 20) ;
  for (byte i = len; i < 20; i++) buffer[i] = ' ';

  block = 12;
  //Serial.println(F("Autenticacao usando chave A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //Grava no bloco 10
  status = mfrc522.MIFARE_Write(block, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  block = 13;
  //Serial.println(F("Authenticating using key A..."));
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,
                                    block, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  //Grava no bloco 12
  status = mfrc522.MIFARE_Write(block, &buffer[16], 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    //return;
  }
  else
  {
    Serial.println(F("Dados gravados com sucesso!"));
    lcd.clear();
    lcd.print("Gravacao OK!");
  }

  mfrc522.PICC_HaltA(); // Halt PICC
  mfrc522.PCD_StopCrypto1();  // Stop encryption on PCD
  delay(5000);
  mensageminicial();
}
