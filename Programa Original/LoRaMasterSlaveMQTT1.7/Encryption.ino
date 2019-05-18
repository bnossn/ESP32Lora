/* Encryption Module*/

#include <AESLib.h>
#include <AES.h>

AESLib aesLib;

uint64_t chipid;

// AES Encryption Key
byte aes_key[] = { 0xC2, 0xE6, 0xCC, 0x86, 0x75, 0xED, 0x2B, 0xBF, 0xAF, 0x10, 0x34, 0x2B, 0x67, 0xA8, 0x11, 0x93 };

// General initialization vector (use your own)
byte aes_iv[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

char cleartext[256];
char ciphertext[1024];

bool bdebug = false;

// Generate IV (once)
void aes_init() {
  char initText[256];
  aesLib.gen_iv(aes_iv);
  sprintf(initText, "AAA");
  encrypt(initText, aes_iv); // workaround for incorrect B64 functionality on first run... initing b64 is not enough
}

// Serves for debug logging the case where IV changes after use...
void print_key_iv(byte iv[]) {

  int i;

  /*
    Serial.println("AES Key: ");
    for (i = 0; i < sizeof(aes_key); i++) {
     Serial.print(aes_key[i], DEC);
     if ((i+1) < sizeof(aes_key)) {
      Serial.print(",");
     }
    }

    Serial.println("");
  */

  Serial.println("AES IV: ");
  for (i = 0; i < sizeof(aes_iv); i++) {
    Serial.print(iv[i]);
    if ((i + 1) < sizeof(aes_iv)) {
      Serial.print(",");
    }
  }

  Serial.println("");
}

String ivToString() {

  String strIV;
  int i;

  for (i = 0; i < sizeof(aes_iv); i++) {
    strIV =  strIV + String(aes_iv[i]);
    if ((i + 1) < sizeof(aes_iv)) {
      strIV = strIV + ",";
    }
  }

  return strIV;
}

void stringToIV (String strIV, byte iv[]) {
  // !!!! Converte a String no array do vetor
  for (int i = 0; i < 16; i++) {
    int tempindex = strIV.indexOf(",");
    if (tempindex > 0) {
      iv[i] = strIV.substring(0, tempindex).toInt();
      strIV = strIV.substring(tempindex + 1); // Retorna o Resto da Mensagem
    } else if (strIV.length() > 0) { //Pega o ultimo valor
      iv[i] = strIV.toInt();
      break;
    }
  }
  // !!!! \Converte a String no array do vetor
}

//char message[200] = {0};

String encode(String msg) {

  char output[256] = {0};
  char input[256] = {0};
  sprintf(input, msg.c_str());

  //int inputLen = strlen(input);
  int enlen = base64_encode(output, input, msg.length());

  Serial.printf("Encoded %i bytes to %s \n", enlen, output);
  //sprintf(message, output);
  return String(output);
}

String decode(String encoded) {
  char output[256] = {0};
  char input[256] = {0};

  sprintf(input, encoded.c_str());

  int msglen = base64_decode(output, input, encoded.length());

  Serial.printf("Decoded %i bytes to %s \n", msglen, output);

  return String(output);
}

String encrypt(char * msg, byte iv[]) {
  unsigned long ms = micros();
  int msgLen = strlen(msg);
  char encrypted[4 * msgLen];
  aesLib.encrypt64(msg, encrypted, aes_key, iv);
  if (bdebug){
    Serial.print("Encryption took: ");
    Serial.print(micros() - ms);
    Serial.println("us");
  }
  return String(encrypted);
}

String decrypt(char * msg, byte iv[]) {
  unsigned long ms = micros();
  int msgLen = strlen(msg);
  char decrypted[msgLen]; // half may be enough
  aesLib.decrypt64(msg, decrypted, aes_key, iv);
  if (bdebug){
    Serial.print("Decryption [2] took: ");
    Serial.print(micros() - ms);
    Serial.println("us");
  }
  return String(decrypted);
}


String decryptMessage (String msgToDecrypt, String deviceID) {

  String msgRec;
  msgRec = msgToDecrypt;

  //Verifica se a string possui "&"
  int index = msgRec.indexOf("&");
  if (index >= 0) { //Primeira parte
    // !!! Primeira parte da string (Tamanho da mensagem)
    String msgRecLen = msgRec.substring(0, index); //Retorna Tamanho da Mensagem
    msgRec = msgRec.substring(index + 1); // Retorna o Resto da Mensagem
    if (bdebug){
      Serial.println("Primeira parte: ");
      Serial.print("  Tamanho: ");
      Serial.println(msgRecLen);
      Serial.print("  Resto: ");
      Serial.println(msgRec);
      Serial.println();
    }
    // !!! \Primeira parte da string


    //Verifica se a string possui "&"
    index = msgRec.indexOf("&");
    if ((index >= 0) && (msgRec.length() == msgRecLen.toInt())) { //Segunda parte - Verifica se mensagem tem tamanho esperado.
      // !!! Segunda parte da string (SlaveID)
      String destID = msgRec.substring(0, index); //Retorna o ID de interesse.
      msgRec = msgRec.substring(index + 1); // Retorna o Resto da Mensagem
      if (bdebug){
        Serial.println("Segunda parte: ");
        Serial.print("  deviceID: ");
        Serial.println(destID);
        Serial.print("  Resto: ");
        Serial.println(msgRec);
        Serial.println();
      }
      // !!! \Segunda parte da string



      //Verifica se a string possui "&"
      index = msgRec.indexOf("&");
      if ( (index >= 0) && (deviceID.equalsIgnoreCase(destID))) { //Terceira parte - Verifica o ID de interesse.
        // !!! Terceira parte da string (initialization vector)
        String str_iv = msgRec.substring(0, index); //Retorna o vetor
        msgRec = msgRec.substring(index + 1); // Retorna o Resto da Mensagem
        if (bdebug){
          Serial.println("Terceira parte: ");
          Serial.print("  IV: ");
          Serial.println(str_iv);
          Serial.print("  Resto: ");
          Serial.println(msgRec);
          Serial.println();
        }
        // !!! \Terceira parte da string



        if (msgRec.length() > 0) { //Quarta parte
          // !!! Quarta parte da string (Mensagem Criptografada
          String encrypted =  msgRec; //Retorna a mensagem criptografada
          if (bdebug){
            Serial.println("Quarta parte: ");
            Serial.print("  Mensagem Criptografada: ");
            Serial.println(encrypted);
            Serial.println();
          }
          // !!! \Quarta parte da string



          // !!!! Decodifica Mensagem
          sprintf(ciphertext, "%s", encrypted.c_str()); //Converting to char[]

          byte dec_iv[16]; // iv_block gets written to, requires always fresh copy
          stringToIV(str_iv, dec_iv);

          if (bdebug) print_key_iv(dec_iv);
          
          String decrypted = decrypt(ciphertext, dec_iv);

          if (bdebug){
            Serial.print("Decrypted Result: ");
            Serial.println(decrypted);
            Serial.println();
          }
          
          return decrypted;
          // !!!! \Decodifica Mensagem

        }
      }
    }
  }
  return "";
}


String encryptMessage(String msgToEncrypt, String deviceID ) {

  aesLib.gen_iv(aes_iv);

  if (bdebug){
    Serial.println("IV Atual: ");
    Serial.println(ivToString());
  }
  //  msg = encode(ivToString());
  //  msg = decode(msg);
  //Serial.println(msg);

  sprintf(cleartext, msgToEncrypt.c_str());

  byte enc_iv[16]; // iv_block gets written to, reqires always fresh copy.
  memcpy(enc_iv, aes_iv, sizeof(aes_iv));
  if (bdebug) print_key_iv(enc_iv);
  String encrypted = encrypt(cleartext, enc_iv);
  if (bdebug){
    Serial.print("Encrypted Result: ");
    Serial.println(encrypted);
    Serial.println();
  }


  /*
     Mensagem codificada como:
     TamanhoMsg & deviceID & initialization vector & mensagem criptografada
     Slave envia mensgem com seu proprio ID
     Master envia mensagem com ID de destino
  */
  String msg;
  msg = encrypted; //mensagem criptografa
  msg = "&" + msg; //marcador 1
  msg = ivToString() + msg; //initialization vector
  msg = "&" + msg; // marcador 2
  msg = deviceID + msg; // Device ID
  uint16_t msglen = msg.length();
  msg = "&" + msg; //marcador 3
  msg = String(msglen) + msg; //Tamanha da mensagem

  if (bdebug){
  Serial.print("Message Encrypted: ");
  Serial.println(msg);
  Serial.println();
  }

  return msg;

}
