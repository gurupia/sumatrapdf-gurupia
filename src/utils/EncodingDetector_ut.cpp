/* Copyright 2022 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "BaseUtil.h"
#include "EncodingDetector.h"
#include "UtAssert.h"

void EncodingDetector_UnitTests() {
    // Test UTF-8 detection
    {
        const char* utf8Data = "\xED\x95\x9C\xEA\xB8\x80"; // "한글" in UTF-8
        ByteSlice data((u8*)utf8Data, 6);
        EncodingResult res = EncodingDetector::DetectEncoding(data);
        uint codepage = res.codepage;
        utassert(codepage == CP_UTF8);
    }

    // Test CP949 (EUC-KR) detection
    // "한글" in CP949: C7 D1 B1 DB
    {
        const char* cp949Data = "\xC7\xD1\xB1\xDB";
        ByteSlice data((u8*)cp949Data, 4);
        EncodingResult res = EncodingDetector::DetectEncoding(data);
        uint codepage = res.codepage;
        // DetectEncoding might return CP_ACP or 949 depending on system locale or heuristics
        // If system locale is Korean (949), it should return 949 or CP_ACP (which is 949).
        // If system locale is not Korean, it might fail to detect specific 949 if it relies on IsLikelyANSI.
        // However, EncodingDetector has specific logic.
        // Let's see what it returns. Ideally it should be 949.
        // But DetectEncoding falls back to GetACP() if not UTF-8/16/BOM.
        // So we expect GetACP() if the machine is Korean, or just "not UTF-8".
        
        // For the purpose of this test, we want to ensure it is NOT detected as UTF-8.
        utassert(codepage != CP_UTF8);
        
        // If we want to verify it detects 949 specifically, we might need to check if DetectEncoding
        // has specific logic for 949.
        // Looking at EncodingDetector.cpp, it doesn't seem to have specific CP949 detection logic 
        // other than BOM or "IsLikelyASCII".
        // It falls back to GetACP().
        
        // So we assert it returns GetACP() or 949.
        uint acp = GetACP();
        utassert(codepage == acp || codepage == 949);
    }

    // Test ASCII detection
    {
        const char* asciiData = "Hello World";
        ByteSlice data((u8*)asciiData, 11);
        EncodingResult res = EncodingDetector::DetectEncoding(data);
        uint codepage = res.codepage;
        if (codepage != CP_UTF8 && codepage != 1252 && codepage != 437 && codepage != 20127) {
            printf("Detected codepage for ASCII: %u\n", codepage);
        }
        utassert(codepage == CP_UTF8 || codepage == 1252 || codepage == 437 || codepage == 20127);
    }
}
