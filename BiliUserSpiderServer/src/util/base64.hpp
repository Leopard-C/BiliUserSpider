#ifndef __BASE64_H__
#define __BASE64_H__

#include <string>
#include <fstream>


class Base64 {
public:
    std::string encode(const std::string& str) {
        const char* bytes_to_encode = str.c_str();
        int in_len = str.length();
        std::string ret;
        int i = 0;
        int j = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];

        while (in_len--) {
            char_array_3[i++] = *(bytes_to_encode++);
            if(i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;     
                for(i = 0; (i <4) ; i++) {
                    ret += base64_chars[char_array_4[i]];
                }
                i = 0;
            }
        }
        if(i) {
            for(j = i; j < 3; j++) {
                char_array_3[j] = '\0';
            }

            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(j = 0; (j < i + 1); j++) {
                ret += base64_chars[char_array_4[j]];
            }

            while((i++ < 3)) {
                ret += '=';
            }

        }
        return ret;
    }

    std::string decode(const std::string& encoded_string) {
        int in_len = (int)encoded_string.size();
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        std::string ret;

        while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
            char_array_4[i++] = encoded_string[in_]; in_++;
            if (i ==4) {
                for (i = 0; i <4; i++)
                    char_array_4[i] = base64_chars.find(char_array_4[i]);

                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

                for (i = 0; (i < 3); i++)
                    ret += char_array_3[i];
                i = 0;
            }
        }
        if (i) {
            for (j = i; j <4; j++)
                char_array_4[j] = 0;

            for (j = 0; j <4; j++)
                char_array_4[j] = base64_chars.find(char_array_4[j]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);  
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];  

            for (j = 0; (j < i - 1); j++)
                ret += char_array_3[j];  
        }  

        return ret;  
    }

    std::string imageEncode(const std::string& filename) {
        std::ifstream ifs;
        ifs.open(filename, std::ios::in | std::ios::binary);
        if (!ifs.is_open()) {
            return "";
        }
        std::string str((std::istreambuf_iterator<char>(ifs)),
                 std::istreambuf_iterator<char>());
        ifs.close();
        std::string imgBase64 = encode(str);
        return imgBase64;
    }

    bool imageDecode(const std::string& base64Str, const std::string& outFilename) {
        std::ofstream ofs;
        ofs.open(outFilename, std::ios::out | std::ios::binary);
        if (!ofs.is_open()) {
            return false;
        }
        std::string imgDecode64 = decode(base64Str);
        ofs << imgDecode64;
        return true;
    }

private:
    static const std::string base64_chars;

    static inline bool is_base64(const char c) {
        return (isalnum(c) || (c == '+') || (c == '/'));
    }
};

const std::string Base64::base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

#endif


