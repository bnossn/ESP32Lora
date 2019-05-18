/* Encryption Module*/

#include <aes2.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Enable ECB, CTR and CBC mode. Note this can be done before including aes.h or at compile-time.
// E.g. with GCC by using the -D flag: gcc -c aes.c -DCBC=0 -DCTR=1 -DECB=1
#define CBC 1
#define CTR 1
#define ECB 1

// AES Encryption Key
uint8_t aes_key[16] = { 0xC2, 0xE6, 0xCC, 0x86, 0x75, 0xED, 0x2B, 0xBF, 0xAF, 0x10, 0x34, 0x2B, 0x67, 0xA8, 0x11, 0x93 };

// General initialization vector (use your own)
uint8_t aes_iv[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

struct Header {
  byte AES_iv[16];
  uint64_t idOrigem;
  char padding[8];
};

struct gtwData {
  char gtwMsg[32];
  uint64_t idDestino;
  uint16_t imsgCounter;
  bool bRstCounter;
};

struct slaveData {
  uint16_t AO[2];
  uint16_t AI[6];
  uint64_t idDestino;
  uint16_t imsgCounter;
  uint16_t DigIn;
  uint16_t DigOut;
};

bool bEncryptDebug = false;

//Parameters = Object, Sizeof(Object), Key, IV
void* encryptData(void *clearData, size_t objSize, uint8_t key[], uint8_t iv[]) //Criptografa e retorna
{
  int memSizeWithPadding; //Memory data size must be evenly divisible by 16byte
  void *cypherData;

  // !!! Take care of padding
  memSizeWithPadding = objSize;
  while (!(memSizeWithPadding % 16 == 0)) memSizeWithPadding++; //add padding to the end of the data
  // !!! \Take care of padding

  Serial_Print(bDebug, "Allocated memory with padding: "); 
  sprintf (Sbuffer, "%i", memSizeWithPadding);
  Serial_Println(bDebug, Sbuffer);  
  
  cypherData = (void*) malloc(memSizeWithPadding);
  
  memcpy(cypherData, clearData, objSize); //Creates a copy of the data to encrypt;
                    
  struct AES_ctx ctx;

  printKey(iv, AES_BLOCKLEN);
  
  AES_init_ctx_iv(&ctx, key, iv);
  AES_CBC_encrypt_buffer(&ctx,(uint8_t*) cypherData, memSizeWithPadding);

  Serial_Println(bDebug, "CBC encrypted!"); 
 
  return cypherData;
}

//Parameters = Object, Sizeof(Object), Key, IV
void* decryptData(void *cypherData, size_t objSize, uint8_t key[], uint8_t iv[]) //Decriptograda e retorna
{
  int memSizeWithPadding; //Memory data size must be evenly divisible by 16byte
  void *clearData;

  // !!! Take care of padding
  memSizeWithPadding = objSize;
  while (!(memSizeWithPadding % 16 == 0)) memSizeWithPadding++; //add padding to the end of the data
  // !!! \Take care of padding

  Serial_Print(bDebug, "Allocated memory with padding: "); 
  sprintf (Sbuffer, "%i", memSizeWithPadding);
  Serial_Println(bDebug, Sbuffer);  
  
  clearData = (void*) malloc(memSizeWithPadding);
  
  memcpy(clearData, cypherData, objSize); //Creates a copy of the data to encrypt;
                    
  struct AES_ctx ctx;

  printKey(iv, AES_BLOCKLEN);
  
  AES_init_ctx_iv(&ctx, key, iv);
  AES_CBC_decrypt_buffer(&ctx,(uint8_t*) clearData, memSizeWithPadding);

  Serial_Println(bDebug, "CBC decrypted!"); 
 
  return clearData;
}


static void printKey(uint8_t iv[], int len){
  
   for (int i=0; i < len; i++) {
      Serial_Print(bDebug, "IV: ");
      sprintf (Sbuffer, "%X", iv[i]);
      Serial_Print(bDebug, Sbuffer);
      Serial_Print(bDebug, " - ");
    }
    Serial_Println(bDebug, "");

}

//Congela e restarta programa se o tamanho dos structs nao for multiplo de 16 bytes.
void checkStructPadding(){

  if (((sizeof(struct Header) % 16) != 0) || 
      ((sizeof(struct gtwData) % 16) != 0) ||
          ((sizeof(struct slaveData) % 16) != 0)){

            displayMessage(130);
            while(true){yield();}
            
          }
  
}
