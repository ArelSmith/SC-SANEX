#include "base64.h"
#include "vigenere.h"

static std::string encrypt_key(std::string &msg, std::string &key) {
	std::string b64_str = base64_encode(msg);
	std::string vigenere_msg = encrypt_vigenere(b64_str, key);
	return vigenere_msg;
}

static std::string decrypt_key(std::string &encrypted_msg, std::string &key) {
	std::string newKey = extend_key(encrypted_msg, key);
	std::string b64_encoded_str = decrypt_vigenere(encrypted_msg, newKey);
	std::string b64_decode_str = base64_decode(b64_encoded_str);
	return b64_decode_str;
}