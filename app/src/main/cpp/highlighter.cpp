#include "highlighter.h"
#include <unordered_set>
#include <cctype>

static const std::unordered_set<std::string> KEYWORDS = {
    // C / C++ keywords
    "alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit",
    "atomic_noexcept", "auto", "bitand", "bitor", "bool", "break", "case", "catch",
    "char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const",
    "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await",
    "co_return", "co_yield", "decltype", "default", "delete", "do", "double",
    "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float",
    "for", "friend", "goto", "if", "inline", "int", "long", "mutable", "namespace",
    "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq",
    "override", "private", "protected", "public", "register", "reinterpret_cast",
    "requires", "return", "short", "signed", "sizeof", "static", "static_assert",
    "static_cast", "struct", "switch", "template", "this", "thread_local", "throw",
    "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using",
    "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq",
    // Java keywords (not already in C++)
    "abstract", "assert", "boolean", "byte", "extends", "final", "finally",
    "implements", "import", "instanceof", "interface", "native", "null", "package",
    "strictfp", "super", "synchronized", "throws", "transient"
};

std::vector<HighlightSpan> highlightCode(const std::string& text) {
    std::vector<HighlightSpan> spans;
    const size_t n = text.size();
    if (n == 0) {
        return spans;
    }

    // Precompute UTF-8 byte offset to UTF-16 code unit offset mapping
    std::vector<int> byte_to_u16(n + 1, 0);
    int u16 = 0;
    size_t idx = 0;
    while (idx < n) {
        byte_to_u16[idx] = u16;
        unsigned char c = static_cast<unsigned char>(text[idx]);
        size_t char_bytes = 1;
        int u16_adv = 1;
        if (c <= 0x7F) {
            char_bytes = 1;
            u16_adv = 1;
        } else if ((c & 0xE0) == 0xC0) {
            char_bytes = 2;
            u16_adv = 1;
        } else if ((c & 0xF0) == 0xE0) {
            char_bytes = 3;
            u16_adv = 1;
        } else if ((c & 0xF8) == 0xF0) {
            char_bytes = 4;
            u16_adv = 2; // Surrogate pair in UTF-16
        } else {
            char_bytes = 1;
            u16_adv = 1;
        }
        if (idx + char_bytes > n) {
            char_bytes = n - idx;
        }
        for (size_t b = 1; b < char_bytes; ++b) {
            byte_to_u16[idx + b] = u16;
        }
        idx += char_bytes;
        u16 += u16_adv;
    }
    byte_to_u16[n] = u16;

    auto addSpan = [&](size_t start_byte, size_t end_byte, int type) {
        int u16_start = byte_to_u16[start_byte];
        int u16_end = byte_to_u16[end_byte];
        if (u16_end > u16_start) {
            spans.push_back({ u16_start, u16_end, type });
        }
    };

    size_t i = 0;
    while (i < n) {
        char c = text[i];
        if (static_cast<unsigned char>(c) <= 32) {
            ++i;
            continue;
        }

        // 1. Comments: line comments and block comments
        if (c == '/' && i + 1 < n) {
            if (text[i + 1] == '/') {
                size_t start = i;
                i += 2;
                while (i < n && text[i] != '\n') {
                    ++i;
                }
                addSpan(start, i, TOKEN_COMMENT);
                continue;
            } else if (text[i + 1] == '*') {
                size_t start = i;
                i += 2;
                while (i + 1 < n && !(text[i] == '*' && text[i + 1] == '/')) {
                    ++i;
                }
                if (i + 1 < n) {
                    i += 2;
                } else {
                    i = n;
                }
                addSpan(start, i, TOKEN_COMMENT);
                continue;
            }
        }

        // 2. String and character literals
        if (c == '"' || c == '\'') {
            char quote = c;
            size_t start = i;
            ++i;
            while (i < n && text[i] != quote && text[i] != '\n') {
                if (text[i] == '\\' && i + 1 < n) {
                    i += 2; // Skip escaped character
                } else {
                    ++i;
                }
            }
            if (i < n && text[i] == quote) {
                ++i;
            }
            addSpan(start, i, TOKEN_STRING);
            continue;
        }

        // 3. Numbers (hex, binary, decimal, float, scientific notation)
        if (std::isdigit(static_cast<unsigned char>(c)) ||
            (c == '.' && i + 1 < n && std::isdigit(static_cast<unsigned char>(text[i + 1])))) {
            size_t start = i;
            if (c == '0' && i + 1 < n && (text[i + 1] == 'x' || text[i + 1] == 'X')) {
                i += 2;
                while (i < n && std::isxdigit(static_cast<unsigned char>(text[i]))) {
                    ++i;
                }
            } else if (c == '0' && i + 1 < n && (text[i + 1] == 'b' || text[i + 1] == 'B')) {
                i += 2;
                while (i < n && (text[i] == '0' || text[i] == '1')) {
                    ++i;
                }
            } else {
                while (i < n && std::isdigit(static_cast<unsigned char>(text[i]))) {
                    ++i;
                }
                if (i < n && text[i] == '.' && (i + 1 == n || text[i + 1] != '.')) {
                    ++i;
                    while (i < n && std::isdigit(static_cast<unsigned char>(text[i]))) {
                        ++i;
                    }
                }
                if (i < n && (text[i] == 'e' || text[i] == 'E')) {
                    size_t e_pos = i;
                    ++i;
                    if (i < n && (text[i] == '+' || text[i] == '-')) {
                        ++i;
                    }
                    if (i < n && std::isdigit(static_cast<unsigned char>(text[i]))) {
                        while (i < n && std::isdigit(static_cast<unsigned char>(text[i]))) {
                            ++i;
                        }
                    } else {
                        i = e_pos; // Rollback if incomplete exponent
                    }
                }
            }
            while (i < n && (text[i] == 'f' || text[i] == 'F' ||
                             text[i] == 'l' || text[i] == 'L' ||
                             text[i] == 'u' || text[i] == 'U' ||
                             text[i] == 'd' || text[i] == 'D' ||
                             text[i] == 'z' || text[i] == 'Z')) {
                ++i;
            }
            addSpan(start, i, TOKEN_NUMBER);
            continue;
        }

        // 4. Identifiers and Keywords
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            size_t start = i;
            while (i < n && (std::isalnum(static_cast<unsigned char>(text[i])) || text[i] == '_')) {
                ++i;
            }
            std::string ident = text.substr(start, i - start);
            if (KEYWORDS.find(ident) != KEYWORDS.end()) {
                addSpan(start, i, TOKEN_KEYWORD);
            }
            continue;
        }

        ++i;
    }

    return spans;
}
