/* Copyright 2022 the GurupiaReader project authors (see AUTHORS file).
   License: Simplified BSD (see COPYING.BSD) */

namespace webp {

bool HasSignature(const ByteSlice&);
Size SizeFromData(const ByteSlice&);
Gdiplus::Bitmap* ImageFromData(const ByteSlice&);

} // namespace webp
