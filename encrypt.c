#include <stdio.h>
#include <string.h>
#define BLOCK_SIZE 5
#include <stdlib.h>
#include <assert.h>

/* this is the default LibTomCrypt macro  */

/* error codes [will be expanded in future releases] */
enum {
    CRYPT_OK = 0, /* Result OK */
    CRYPT_ERROR, /* Generic Error */
    CRYPT_NOP, /* Not a failure but no operation was performed */

    CRYPT_INVALID_KEYSIZE, /* Invalid key size given */
    CRYPT_INVALID_ROUNDS, /* Invalid number of rounds */
    CRYPT_FAIL_TESTVECTOR, /* Algorithm failed test vectors */

    CRYPT_BUFFER_OVERFLOW, /* Not enough space for output */
    CRYPT_INVALID_PACKET, /* Invalid input packet given */

    CRYPT_INVALID_PRNGSIZE, /* Invalid number of bits for a PRNG */
    CRYPT_ERROR_READPRNG, /* Could not read enough from PRNG */

    CRYPT_INVALID_CIPHER, /* Invalid cipher specified */
    CRYPT_INVALID_HASH, /* Invalid hash specified */
    CRYPT_INVALID_PRNG, /* Invalid PRNG specified */

    CRYPT_MEM, /* Out of memory */

    CRYPT_PK_TYPE_MISMATCH, /* Not equivalent types of PK keys */
    CRYPT_PK_NOT_PRIVATE, /* Requires a private PK key */

    CRYPT_INVALID_ARG, /* Generic invalid argument */
    CRYPT_FILE_NOTFOUND, /* File Not Found */

    CRYPT_PK_INVALID_TYPE, /* Invalid type of PK key */
    CRYPT_PK_INVALID_SYSTEM, /* Invalid PK system specified */
    CRYPT_PK_DUP, /* Duplicate key already in key ring */
    CRYPT_PK_NOT_FOUND, /* Key not found in keyring */
    CRYPT_PK_INVALID_SIZE, /* Invalid size input for PK parameters */

    CRYPT_INVALID_PRIME_SIZE, /* Invalid size of prime requested */
    CRYPT_PK_INVALID_PADDING, /* Invalid padding on input */

    CRYPT_HASH_OVERFLOW /* Hash applied to too many bits */
#ifdef MACTOMCRYPT
    , CRYPT_UNIMPLEMENTED /* called an unimplemented routine through a function table */
#endif
};

static const unsigned char map[256] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 62, 255, 255, 255, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 255, 255,
    255, 254, 255, 255, 255, 0, 1, 2, 3, 4, 5, 6,
    7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 255, 255, 255, 255, 255,
    255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
    37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255
};

/**
   base64 decode a block of memory
   @param in       The base64 data to decode
   @param inlen    The length of the base64 data
   @param out      [out] The destination of the binary decoded data
   @param outlen   [in/out] The max size and resulting size of the decoded data
   @return CRYPT_OK if successful
 */
int base64_decode(const unsigned char *in, unsigned long inlen,
        unsigned char *out, unsigned long *outlen) {
    unsigned long t, x, y, z;
    unsigned char c;
    int g;

    assert(in != NULL);
    assert(out != NULL);
    assert(outlen != NULL);

    g = 3;
    for (x = y = z = t = 0; x < inlen; x++) {
        c = map[in[x]&0xFF];
        if (c == 255) continue;
        /* the final = symbols are read and used to trim the remaining bytes */
        if (c == 254) {
            c = 0;
            /* prevent g < 0 which would potentially allow an overflow later */
            if (--g < 0) {
                return CRYPT_INVALID_PACKET;
            }
        } else if (g != 3) {
            /* we only allow = to be at the end */
            return CRYPT_INVALID_PACKET;
        }

        t = (t << 6) | c;

        if (++y == 4) {
            if (z + g > *outlen) {
                return CRYPT_BUFFER_OVERFLOW;
            }
            out[z++] = (unsigned char) ((t >> 16)&255);
            if (g > 1) out[z++] = (unsigned char) ((t >> 8)&255);
            if (g > 2) out[z++] = (unsigned char) (t & 255);
            y = t = 0;
        }
    }
    if (y != 0) {
        return CRYPT_INVALID_PACKET;
    }
    *outlen = z;
    return CRYPT_OK;
}


static const char *codes =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
   base64 Encode a buffer (NUL terminated)
   @param in      The input buffer to encode
   @param inlen   The length of the input buffer
   @param out     [out] The destination of the base64 encoded data
   @param outlen  [in/out] The max size and resulting size
   @return CRYPT_OK if successful
 */

int base64_encode(const unsigned char *in, unsigned long inlen,
        unsigned char *out, unsigned long *outlen) {
    unsigned long i, len2, leven;
    unsigned char *p;

    assert(in != NULL);
    assert(out != NULL);
    assert(outlen != NULL);

    /* valid output size ? */
    len2 = 4 * ((inlen + 2) / 3);
    if (*outlen < len2 + 1) {
        *outlen = len2 + 1;
        return CRYPT_BUFFER_OVERFLOW;
    }
    p = out;
    leven = 3 * (inlen / 3);
    for (i = 0; i < leven; i += 3) {
        *p++ = codes[(in[0] >> 2) & 0x3F];
        *p++ = codes[(((in[0] & 3) << 4) + (in[1] >> 4)) & 0x3F];
        *p++ = codes[(((in[1] & 0xf) << 2) + (in[2] >> 6)) & 0x3F];
        *p++ = codes[in[2] & 0x3F];
        in += 3;
    }
    /* Pad it if necessary...  */
    if (i < inlen) {
        unsigned a = in[0];
        unsigned b = (i + 1 < inlen) ? in[1] : 0;

        *p++ = codes[(a >> 2) & 0x3F];
        *p++ = codes[(((a & 3) << 4) + (b >> 4)) & 0x3F];
        *p++ = (i + 1 < inlen) ? codes[(((b & 0xf) << 2)) & 0x3F] : '=';
        *p++ = '=';
    }

    /* append a NULL byte */
    *p = '\0';

    /* return ok */
    *outlen = p - out;
    return CRYPT_OK;
}

void encode(char *in, char *key, char* out) {

    unsigned char *p;
    p = out;
    for (int i = 0; i < strlen(in); i++) {
        char inchar = in[i];
        char keychar = key[(i % strlen(key)) - 1];
        char r = (char) ((int) inchar + (int) keychar);
        *p++ = r;
    }
}

void decode(char *in, char *key, char* out) {

    unsigned char *p;
    p = out;
    for (int i = 0; i < strlen(in); i++) {
        char inchar = in[i];

        char keychar = key[(i % strlen(key)) - 1];
        char r = (char) ((int) inchar - (int) keychar);
        *p++ = r;
    }

}

int encode64(char *in, char *key, char* out, unsigned long *outlen) {
    unsigned char encoded[*outlen];
    encode(in, key, encoded);
    
    int result = base64_encode(encoded, strlen(in), out, outlen);
    return result;


}

int decode64(char *in, char *key, char* out, unsigned long inlen, unsigned long* outlen) {
    unsigned char decoded[inlen];

    int result = base64_decode(in, inlen, decoded, outlen);
    decode(decoded, key, out);
    return result;
}

/* Example usage */
int main() {
    const unsigned char data[] = {"Thank you. It is my birthday"};
    
    char key[] = {"my secure key"}; // 64 characters

    unsigned char encoded[256];
    unsigned char decoded[256];
    unsigned long outlen = 256;
    unsigned long outlen2 = 256;

    int result = encode64(data, key, encoded, &outlen);
    int decresl = decode64(encoded, key, decoded, outlen, &outlen2);
    printf("%d encoded [ %s ] %d decoded [ %s ]\n", result, encoded, decresl, decoded);
    return 0;
}
