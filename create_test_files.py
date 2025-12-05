
# -*- coding: utf-8 -*-
import os

def create_files():
    # Korean text "안녕하세요"
    korean_text = "안녕하세요"
    
    # CP949 (EUC-KR)
    with open("korean_cp949.txt", "wb") as f:
        f.write(korean_text.encode("cp949"))
    print("Created korean_cp949.txt")
    
    # UTF-8
    with open("korean_utf8.txt", "wb") as f:
        f.write(korean_text.encode("utf-8"))
    print("Created korean_utf8.txt")
    
    # ASCII
    with open("ascii.txt", "wb") as f:
        f.write(b"Hello World")
    print("Created ascii.txt")

if __name__ == "__main__":
    create_files()
