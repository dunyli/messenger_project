/*
 * crypto.c — реализация криптографических функций
 *
 * Использует OpenSSL EVP API (современный интерфейс).
 * SHA-256 выбран как компромисс между безопасностью и производительностью.
 */

#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include "crypto.h"

/*
 * sha256_hash — вычисляет SHA-256 хеш и возвращает в hex-формате
 * Пример: "hello" -> "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"
 */
void sha256_hash(const char *input, char *output) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digest_len = 0;

    // Создаём контекст для SHA-256
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, input, strlen(input));
    EVP_DigestFinal_ex(ctx, digest, &digest_len);
    EVP_MD_CTX_free(ctx);

    // Конвертируем бинарный хеш в hex-строку
    for (unsigned int i = 0; i < digest_len; i++) {
        sprintf(output + (i * 2), "%02x", digest[i]);
    }
    output[digest_len * 2] = '\0';
}