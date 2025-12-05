/* Copyright 2022 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

// EncodingDetector provides automatic detection of text file encodings
// using BOM detection, declaration parsing, and heuristic analysis

enum class EncodingConfidence {
    None = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Certain = 4
};

struct EncodingResult {
    uint codepage;
    EncodingConfidence confidence;
    const char* encodingName;

    EncodingResult() : codepage(0), confidence(EncodingConfidence::None), encodingName(nullptr) {
    }

    EncodingResult(uint cp, EncodingConfidence conf, const char* name = nullptr)
        : codepage(cp), confidence(conf), encodingName(name) {
    }
};

class EncodingDetector {
  public:
    // Detect encoding from BOM (Byte Order Mark)
    static EncodingResult DetectFromBOM(const ByteSlice& data);

    // Detect encoding from XML/HTML declaration
    static EncodingResult DetectFromDeclaration(const ByteSlice& data);

    // Detect encoding from statistical content analysis
    static EncodingResult DetectFromContent(const ByteSlice& data);

    // Comprehensive detection (tries all methods in order)
    static EncodingResult DetectEncoding(const ByteSlice& data);

    // Get multiple encoding candidates with confidence scores
    static Vec<EncodingResult> DetectMultiple(const ByteSlice& data);

    // Check if data is valid UTF-8
    static bool IsValidUTF8(const ByteSlice& data);

    // Check if data is likely ASCII
    static bool IsLikelyASCII(const ByteSlice& data);

  private:
    // Score a specific codepage for the given data
    static float ScoreCodepage(const ByteSlice& data, uint codepage);

    // Detect from HTML meta tags
    static EncodingResult DetectFromHTMLMeta(const ByteSlice& data);

    // Detect from XML processing instruction
    static EncodingResult DetectFromXMLPI(const ByteSlice& data);
};
