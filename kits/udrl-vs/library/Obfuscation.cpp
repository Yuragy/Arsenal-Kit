#include "Obfuscation.h"
#include "StdLib.h"
#include "LoaderTypes.h"

/*******************************************************************
 * To avoid problems with function positioning, do not add any new
 * functions above this pragma directive.
********************************************************************/
#pragma code_seg(".text$f")

/**
 * XOR the provided buffer with the provided key
 *
 * @param buffer A pointer to the buffer that requires masking/unmasking.
 * @param size The size of the buffer
 * @param key The key to XOR the buffer
 * @return A Boolean value to indicate success
*/
BOOL XORData(char* buffer, size_t size, char* key, size_t keyLength) {
    if (buffer == NULL || key == NULL || keyLength == 0) {
        return FALSE;
    }

    for (size_t index = 0; index < size; index++) {
        buffer[index] ^= key[index % keyLength];
    }
    return TRUE;
}

/**
 * RC4 encrypt/decrypt an input buffer
 * Referenced -  https://gist.github.com/rverton/a44fc8ca67ab9ec32089
 * 
 * @param buffer A pointer to the buffer that requires encryption/decryption
 * @param size The size of the buffer
 * @param key The key to encrypt/decrypt the buffer
 * @param keyLength The length of the encryption key
 * @return A Boolean value to indicate success
*/
#pragma optimize( "", off )
BOOL RC4(unsigned char* buffer, DWORD size, unsigned char* key, DWORD keyLength) {
    if (buffer == NULL || size == 0 || key == NULL || keyLength == 0) {
        return FALSE;
    }
    unsigned char S[256];

    for (int i = 0; i < 256; i++) {
        S[i] = i;
    }

    for (int i = 0, j = 0; i < 256; i++) {
        j = (j + S[i] + ((unsigned char*)key)[i % keyLength]) % 256;
        unsigned char tmp = S[i];
        S[i] = S[j];
        S[j] = tmp;
    }

    int i = 0;
    int j = 0;

    for (size_t n = 0, len = size; n < len; n++) {
        i = (i + 1) % 256;
        j = (j + S[i]) % 256;
        unsigned char tmp = S[i];
        S[i] = S[j];
        S[j] = tmp;
        int rnd = S[(S[i] + S[j]) % 256];
        ((unsigned char*)buffer)[n] = rnd ^ ((unsigned char*)buffer)[n];
    }

    return TRUE;
}
#pragma optimize( "", on )

/**
 * Base64 decode a given input buffer
 * Referenced - https://github.com/elzoughby/Base64
 *
 * @param inputBuffer A pointer to a base64 encoded buffer
 * @param inputLength The size of the input buffer
 * @param outputBuffer A pointer to a buffer that should store the output
 * @return A Boolean value to indicate success
*/
BOOL Base64Decode(char* inputBuffer, DWORD inputLength, char* outputBuffer) {
    if (inputBuffer == NULL || inputLength == 0 || outputBuffer == NULL) {
        return FALSE;
    }
    char characterCount = 0;
    char tmp[4] = { 0 };

    // Generate base64 alphabet at runtime to ensure it is position independent
    char base64Alphabet[64];
    int mapCounter = 0;
    for (char i = 'A'; i <= 'Z'; i++) {
        base64Alphabet[mapCounter++] = i;
    }
    for (char i = 'a'; i <= 'z'; i++) {
        base64Alphabet[mapCounter++] = i;
    }
    for (char i = '0'; i <= '9'; i++) {
        base64Alphabet[mapCounter++] = i;
    }
    base64Alphabet[mapCounter++] = '+';
    base64Alphabet[mapCounter++] = '/';

    for (DWORD i = 0; i < inputLength; i++) {
        char encodedValue;
        for (encodedValue = 0; encodedValue < 64 && base64Alphabet[encodedValue] != inputBuffer[i]; encodedValue++);
        tmp[characterCount++] = encodedValue;
        if (characterCount == 4) {
            *outputBuffer++ = (tmp[0] << 2) + (tmp[1] >> 4);
            if (tmp[2] != 64)
                *outputBuffer++ = (tmp[1] << 4) + (tmp[2] >> 2);
            if (tmp[3] != 64)
                *outputBuffer++ = (tmp[2] << 6) + tmp[3];
            characterCount = 0;
         }
    }
    return TRUE;
}

