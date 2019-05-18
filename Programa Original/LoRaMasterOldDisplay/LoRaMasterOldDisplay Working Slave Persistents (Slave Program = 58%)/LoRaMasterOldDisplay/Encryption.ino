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

bool bEncDebug = true;

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

  Serial_Println(bDebug, "AES IV: ");
  for (i = 0; i < sizeof(aes_iv); i++) {
    sprintf (Sbuffer, "%i", iv[i]);
    Serial_Print(bDebug, Sbuffer);
    if ((i + 1) < sizeof(aes_iv)) {
      Serial_Print(bDebug, ",");
    }
  }

  Serial_Println(bDebug, "");
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

  sprintf (Sbuffer, "Encoded %i bytes to %s \n", enlen, output);
  Serial_Println(bDebug, Sbuffer);
  
  //sprintf(message, output);
  return String(output);
}

String decode(String encoded) {
  char output[256] = {0};
  char input[256] = {0};

  sprintf(input, encoded.c_str());

  int msglen = base64_decode(output, input, encoded.length());

  
  sprintf (Sbuffer, "Decoded %i bytes to %s \n", msglen, output);
  Serial_Println(bDebug, Sbuffer);

  return String(output);
}

String encrypt(char * msg, byte iv[]) {
  unsigned long ms = micros();
  int msgLen = strlen(msg);
  char encrypted[4 * msgLen];
  if (bEncDebug) Serial_Println(bDebug, "ANTES ENCRYPT: ");
  aesLib.encrypt64(msg, encrypted, aes_key, iv);
  if (bEncDebug) Serial_Println(bDebug, "APÓS ENCRYPT: ");
  if (bEncDebug){
    Serial_Print(bDebug, "Encryption took: ");
    sprintf (Sbuffer, "%lu", (micros() - ms));
    Serial_Print(bDebug, Sbuffer);
    Serial_Println(bDebug, "us");
  }
  return String(encrypted);
}

String decrypt(char * msg, byte iv[]) {
  unsigned long ms = micros();
  int msgLen = strlen(msg);
  char decrypted[msgLen]; // half may be enough
  aesLib.decrypt64(msg, decrypted, aes_key, iv);
  if (bEncDebug){
    Serial_Print(bDebug, "Decryption [2] took: ");
    sprintf (Sbuffer, "%lu", (micros() - ms));
    Serial_Print(bDebug, Sbuffer);
    Serial_Println(bDebug, "us");
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
    if (bEncDebug){
      Serial_Println(bDebug, "Primeira parte: ");
      Serial_Print(bDebug, "  Tamanho: ");
      Serial_Println(bDebug, msgRecLen.c_str());
      Serial_Print(bDebug, "  Resto: ");
      Serial_Println(bDebug, msgRec.c_str());
      Serial_Println(bDebug, "\n" );
    }
    // !!! \Primeira parte da string


    //Verifica se a string possui "&"
    index = msgRec.indexOf("&");
    if ((index >= 0) && (msgRec.length() == msgRecLen.toInt())) { //Segunda parte - Verifica se mensagem tem tamanho esperado.
      // !!! Segunda parte da string (SlaveID)
      String destID = msgRec.substring(0, index); //Retorna o ID de interesse.
      msgRec = msgRec.substring(index + 1); // Retorna o Resto da Mensagem
      if (bEncDebug){
        Serial_Println(bDebug, "Segunda parte: ");
        Serial_Print(bDebug, "  deviceID: ");
        Serial_Println(bDebug, destID.c_str());
        Serial_Print(bDebug, "  Resto: ");
        Serial_Println(bDebug, msgRec.c_str());
        Serial_Println(bDebug, "\n" );
      }
      // !!! \Segunda parte da string



      //Verifica se a string possui "&"
      index = msgRec.indexOf("&");
      if ( (index >= 0) && (deviceID.equalsIgnoreCase(destID))) { //Terceira parte - Verifica o ID de interesse.
        // !!! Terceira parte da string (initialization vector)
        String str_iv = msgRec.substring(0, index); //Retorna o vetor
        msgRec = msgRec.substring(index + 1); // Retorna o Resto da Mensagem
        if (bEncDebug){
          Serial_Println(bDebug, "Terceira parte: ");
          Serial_Print(bDebug, "  IV: ");
          Serial_Println(bDebug, str_iv.c_str());
          Serial_Print(bDebug, "  Resto: ");
          Serial_Println(bDebug, msgRec.c_str());
          Serial_Println(bDebug, "\n" );
        }
        // !!! \Terceira parte da string



        if (msgRec.length() > 0) { //Quarta parte
          // !!! Quarta parte da string (Mensagem Criptografada
          String encrypted =  msgRec; //Retorna a mensagem criptografada
          if (bEncDebug){
            Serial_Println(bDebug, "Quarta parte: ");
            Serial_Print(bDebug, "  Mensagem Criptografada: ");
            Serial_Println(bDebug, encrypted.c_str());
            Serial_Println(bDebug, "\n" );
          }
          // !!! \Quarta parte da string



          // !!!! Decodifica Mensagem
          sprintf(ciphertext, "%s", encrypted.c_str()); //Converting to char[]

          byte dec_iv[16]; // iv_block gets written to, requires always fresh copy
          stringToIV(str_iv, dec_iv);

          if (bEncDebug) print_key_iv(dec_iv);
          
          String decrypted = decrypt(ciphertext, dec_iv);

          if (bEncDebug){
            Serial_Print(bDebug, "Decrypted Result: ");
            Serial_Println(bDebug, decrypted.c_str());
            Serial_Println(bDebug, "\n" );
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

  if (bEncDebug){
    Serial_Println(bDebug, "IV Atual: ");
    Serial_Println(bDebug, ivToString().c_str());
  }

  sprintf(cleartext, msgToEncrypt.c_str());

  byte enc_iv[16]; // iv_block gets written to, reqires always fresh copy.
  memcpy(enc_iv, aes_iv, sizeof(aes_iv));
  if (bEncDebug) print_key_iv(enc_iv);
  String encrypted = encrypt(cleartext, enc_iv);
  if (bEncDebug){
    Serial_Print(bDebug, "Encrypted Result: ");
    Serial_Println(bDebug, encrypted.c_str());
    Serial_Println(bDebug, "\n" );
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

  if (bEncDebug){
  Serial_Print(bDebug, "Message Encrypted: ");
  Serial_Println(bDebug, msg.c_str());
  Serial_Println(bDebug, "\n" );
  }

  return msg;

}
