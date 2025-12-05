/* Copyright 2022 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "utils/BaseUtil.h"
#include "utils/StrUtil.h"
#include "EncodingDetector.h"
#include "EncodingInfo.h"

// BOM (Byte Order Mark) signatures
static const u8 kBOM_UTF8[] = {0xEF, 0xBB, 0xBF};
static const u8 kBOM_UTF16_LE[] = {0xFF, 0xFE};
static const u8 kBOM_UTF16_BE[] = {0xFE, 0xFF};
static const u8 kBOM_UTF32_LE[] = {0xFF, 0xFE, 0x00, 0x00};
static const u8 kBOM_UTF32_BE[] = {0x00, 0x00, 0xFE, 0xFF};

EncodingResult EncodingDetector::DetectFromBOM(const ByteSlice& data) {
    if (data.size() < 2) {
        return EncodingResult();
    }

    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    // Check UTF-32 first (4 bytes) before UTF-16 (2 bytes)
    if (len >= 4) {
        if (memeq(d, kBOM_UTF32_LE, 4)) {
            return EncodingResult(12000, EncodingConfidence::Certain, "UTF-32LE");
        }
        if (memeq(d, kBOM_UTF32_BE, 4)) {
            return EncodingResult(12001, EncodingConfidence::Certain, "UTF-32BE");
        }
    }

    // Check UTF-8 (3 bytes)
    if (len >= 3 && memeq(d, kBOM_UTF8, 3)) {
        return EncodingResult(CP_UTF8, EncodingConfidence::Certain, "UTF-8");
    }

    // Check UTF-16
    if (memeq(d, kBOM_UTF16_LE, 2)) {
        return EncodingResult(1200, EncodingConfidence::Certain, "UTF-16LE");
    }
    if (memeq(d, kBOM_UTF16_BE, 2)) {
        return EncodingResult(1201, EncodingConfidence::Certain, "UTF-16BE");
    }

    return EncodingResult();
}

EncodingResult EncodingDetector::DetectFromXMLPI(const ByteSlice& data) {
    // Look for <?xml encoding="..."?>
    const char* start = (const char*)data.data();
    size_t len = data.size();

    // Limit search to first 1KB
    if (len > 1024) {
        len = 1024;
    }

    const char* xmlDecl = str::Find(start, "<?xml");
    if (!xmlDecl || xmlDecl - start > 100) {
        return EncodingResult();
    }

    const char* encodingAttr = str::Find(xmlDecl, "encoding");
    if (!encodingAttr || encodingAttr - xmlDecl > 200) {
        return EncodingResult();
    }

    // Find the value in quotes
    const char* quote1 = str::FindChar(encodingAttr, '"');
    if (!quote1) {
        quote1 = str::FindChar(encodingAttr, '\'');
    }
    if (!quote1 || quote1 - encodingAttr > 20) {
        return EncodingResult();
    }

    quote1++; // Skip opening quote
    const char* quote2 = str::FindChar(quote1, quote1[-1]); // Find matching quote
    if (!quote2 || quote2 - quote1 > 50) {
        return EncodingResult();
    }

    // Extract encoding name
    AutoFree encodingName = str::Dup(quote1, quote2 - quote1);

    // Look up codepage
    uint codepage = EncodingRegistry::GetCodepageByName(encodingName);
    if (codepage != 0) {
        return EncodingResult(codepage, EncodingConfidence::High, encodingName);
    }

    return EncodingResult();
}

EncodingResult EncodingDetector::DetectFromHTMLMeta(const ByteSlice& data) {
    const char* start = (const char*)data.data();
    size_t len = data.size();

    // Limit search to first 2KB (meta tags should be in head)
    if (len > 2048) {
        len = 2048;
    }

    // Look for <meta charset="...">
    const char* metaCharset = str::Find(start, "charset");
    if (metaCharset && metaCharset - start < (int)len) {
        const char* quote1 = str::FindChar(metaCharset, '"');
        if (!quote1) {
            quote1 = str::FindChar(metaCharset, '\'');
        }
        if (!quote1) {
            // Maybe charset=utf-8 without quotes
            const char* eq = str::FindChar(metaCharset, '=');
            if (eq) {
                eq++;
                while (*eq == ' ') {
                    eq++;
                }
                const char* end = eq;
                while (*end && *end != ' ' && *end != '>' && *end != ';') {
                    end++;
                }
                if (end > eq) {
                    AutoFree encodingName = str::Dup(eq, end - eq);
                    uint codepage = EncodingRegistry::GetCodepageByName(encodingName);
                    if (codepage != 0) {
                        return EncodingResult(codepage, EncodingConfidence::High, encodingName);
                    }
                }
            }
        } else {
            quote1++;
            const char* quote2 = str::FindChar(quote1, quote1[-1]);
            if (quote2) {
                AutoFree encodingName = str::Dup(quote1, quote2 - quote1);
                uint codepage = EncodingRegistry::GetCodepageByName(encodingName);
                if (codepage != 0) {
                    return EncodingResult(codepage, EncodingConfidence::High, encodingName);
                }
            }
        }
    }

    return EncodingResult();
}

EncodingResult EncodingDetector::DetectFromDeclaration(const ByteSlice& data) {
    // Try XML first
    EncodingResult result = DetectFromXMLPI(data);
    if (result.confidence != EncodingConfidence::None) {
        return result;
    }

    // Try HTML meta tags
    return DetectFromHTMLMeta(data);
}

bool EncodingDetector::IsValidUTF8(const ByteSlice& data) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();
    size_t i = 0;

    while (i < len) {
        u8 byte = d[i];

        // ASCII (0x00-0x7F)
        if (byte <= 0x7F) {
            i++;
            continue;
        }

        // 2-byte sequence (110xxxxx 10xxxxxx)
        if ((byte & 0xE0) == 0xC0) {
            if (i + 1 >= len) {
                return false;
            }
            if ((d[i + 1] & 0xC0) != 0x80) {
                return false;
            }
            // Check for overlong encoding
            if (byte < 0xC2) {
                return false;
            }
            i += 2;
            continue;
        }

        // 3-byte sequence (1110xxxx 10xxxxxx 10xxxxxx)
        if ((byte & 0xF0) == 0xE0) {
            if (i + 2 >= len) {
                return false;
            }
            if ((d[i + 1] & 0xC0) != 0x80 || (d[i + 2] & 0xC0) != 0x80) {
                return false;
            }
            // Check for overlong encoding and surrogates
            if (byte == 0xE0 && d[i + 1] < 0xA0) {
                return false;
            }
            if (byte == 0xED && d[i + 1] >= 0xA0) {
                return false; // Surrogate
            }
            i += 3;
            continue;
        }

        // 4-byte sequence (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        if ((byte & 0xF8) == 0xF0) {
            if (i + 3 >= len) {
                return false;
            }
            if ((d[i + 1] & 0xC0) != 0x80 || (d[i + 2] & 0xC0) != 0x80 || (d[i + 3] & 0xC0) != 0x80) {
                return false;
            }
            // Check for overlong encoding and values > 0x10FFFF
            if (byte == 0xF0 && d[i + 1] < 0x90) {
                return false;
            }
            if (byte >= 0xF4) {
                return false;
            }
            i += 4;
            continue;
        }

        // Invalid UTF-8
        return false;
    }

    return true;
}

bool EncodingDetector::IsLikelyASCII(const ByteSlice& data) {
    const u8* d = (const u8*)data.data();
    size_t len = data.size();

    // Sample first 4KB
    if (len > 4096) {
        len = 4096;
    }

    size_t nonAsciiCount = 0;
    for (size_t i = 0; i < len; i++) {
        if (d[i] > 0x7F) {
            nonAsciiCount++;
        }
    }

    // If less than 1% non-ASCII, consider it ASCII
    return (nonAsciiCount * 100 / len) < 1;
}

EncodingResult EncodingDetector::DetectFromContent(const ByteSlice& data) {
    // First check if it's valid UTF-8
    if (IsValidUTF8(data)) {
        // Check if it's actually ASCII
        if (IsLikelyASCII(data)) {
            return EncodingResult(20127, EncodingConfidence::High, "ASCII");
        }
        return EncodingResult(CP_UTF8, EncodingConfidence::High, "UTF-8");
    }

    // Sample the data (first 8KB)
    size_t sampleSize = data.size();
    if (sampleSize > 8192) {
        sampleSize = 8192;
    }

    const u8* d = (const u8*)data.data();

    // Simple heuristics for common encodings
    // Count high bytes (0x80-0xFF)
    size_t highByteCount = 0;
    for (size_t i = 0; i < sampleSize; i++) {
        if (d[i] >= 0x80) {
            highByteCount++;
        }
    }

    // If very few high bytes, might be ASCII or Latin-1
    if (highByteCount * 100 / sampleSize < 5) {
        return EncodingResult(1252, EncodingConfidence::Low, "Windows-1252");
    }

    // For now, return system default with low confidence
    // TODO: Implement more sophisticated detection (frequency analysis, etc.)
    uint defaultCP = GetACP();
    return EncodingResult(defaultCP, EncodingConfidence::Low, "System Default");
}

EncodingResult EncodingDetector::DetectEncoding(const ByteSlice& data) {
    if (data.size() == 0) {
        return EncodingResult();
    }

    // 1. Check for BOM (highest confidence)
    EncodingResult result = DetectFromBOM(data);
    if (result.confidence == EncodingConfidence::Certain) {
        return result;
    }

    // 2. Check for encoding declaration (high confidence)
    result = DetectFromDeclaration(data);
    if (result.confidence >= EncodingConfidence::High) {
        return result;
    }

    // 3. Analyze content (medium/low confidence)
    result = DetectFromContent(data);
    return result;
}

Vec<EncodingResult> EncodingDetector::DetectMultiple(const ByteSlice& data) {
    Vec<EncodingResult> results;

    // Add BOM result if present
    EncodingResult bomResult = DetectFromBOM(data);
    if (bomResult.confidence != EncodingConfidence::None) {
        results.Append(bomResult);
        return results; // BOM is certain, no need for other candidates
    }

    // Add declaration result if present
    EncodingResult declResult = DetectFromDeclaration(data);
    if (declResult.confidence != EncodingConfidence::None) {
        results.Append(declResult);
    }

    // Add content analysis result
    EncodingResult contentResult = DetectFromContent(data);
    if (contentResult.confidence != EncodingConfidence::None) {
        // Don't add duplicate
        bool isDuplicate = false;
        for (size_t i = 0; i < results.size(); i++) {
            if (results[i].codepage == contentResult.codepage) {
                isDuplicate = true;
                break;
            }
        }
        if (!isDuplicate) {
            results.Append(contentResult);
        }
    }

    // Add common fallbacks if nothing found
    if (results.size() == 0) {
        results.Append(EncodingResult(CP_UTF8, EncodingConfidence::Low, "UTF-8"));
        results.Append(EncodingResult(GetACP(), EncodingConfidence::Low, "System Default"));
    }

    return results;
}

float EncodingDetector::ScoreCodepage(const ByteSlice& data, uint codepage) {
    // TODO: Implement scoring based on character frequency analysis
    // For now, return a simple score
    return 0.5f;
}
