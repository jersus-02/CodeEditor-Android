#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <string>
#include <vector>

enum TokenType {
    TOKEN_KEYWORD = 0,
    TOKEN_STRING  = 1,
    TOKEN_COMMENT = 2,
    TOKEN_NUMBER  = 3
};

struct HighlightSpan {
    int start; // UTF-16 code unit offset (inclusive)
    int end;   // UTF-16 code unit offset (exclusive)
    int type;  // TokenType: 0 keyword, 1 string literal, 2 comment, 3 number
};

std::vector<HighlightSpan> highlightCode(const std::string& utf8_text);

#endif // HIGHLIGHTER_H
