/*
 * crypto.h — заголовочный файл модуля шифрования
 *
 * Использует OpenSSL для:
 *   - хеширования паролей (SHA-256)
 *   - шифрования сообщений (будет добавлено позже)
 */

#ifndef CRYPTO_H
#define CRYPTO_H

/*
 * sha256_hash — вычисляет SHA-256 хеш строки
 * input  — исходная строка
 * output — буфер для хеша (должен быть не менее 65 байт: 64 символа + '\0')
 */
void sha256_hash(const char *input, char *output);

#endif // CRYPTO_H