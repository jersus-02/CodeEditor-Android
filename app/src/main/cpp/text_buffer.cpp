#include "text_buffer.h"

TextBuffer::TextBuffer() : m_text("") {}

TextBuffer::TextBuffer(const std::string& initial_text) : m_text(initial_text) {}

void TextBuffer::setText(const std::string& text) {
    m_text = text;
    m_undo_stack.clear();
    m_redo_stack.clear();
}

const std::string& TextBuffer::getText() const {
    return m_text;
}

size_t TextBuffer::length() const {
    return m_text.length();
}

int TextBuffer::lineCount() const {
    if (m_text.empty()) {
        return 1;
    }
    int count = 1;
    for (char c : m_text) {
        if (c == '\n') {
            count++;
        }
    }
    return count;
}

bool TextBuffer::replace(size_t pos, size_t len, const std::string& str) {
    if (pos > m_text.size()) {
        pos = m_text.size();
    }
    if (pos + len > m_text.size()) {
        len = m_text.size() - pos;
    }

    std::string old_str = m_text.substr(pos, len);
    if (old_str == str) {
        return true;
    }

    m_undo_stack.push_back({ pos, old_str, str });
    m_redo_stack.clear();

    m_text.replace(pos, len, str);
    return true;
}

bool TextBuffer::insert(size_t pos, const std::string& str) {
    return replace(pos, 0, str);
}

bool TextBuffer::erase(size_t pos, size_t len) {
    return replace(pos, len, "");
}

size_t TextBuffer::utf16ToByteIndex(size_t u16_target) const {
    size_t u16_cur = 0;
    size_t byte_pos = 0;
    const size_t n = m_text.size();

    while (byte_pos < n && u16_cur < u16_target) {
        unsigned char c = static_cast<unsigned char>(m_text[byte_pos]);
        size_t char_bytes = 1;
        size_t u16_adv = 1;
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

        if (byte_pos + char_bytes > n) {
            char_bytes = n - byte_pos;
        }

        byte_pos += char_bytes;
        u16_cur += u16_adv;
    }
    return byte_pos;
}

size_t TextBuffer::byteToUtf16Index(size_t byte_target) const {
    size_t u16_cur = 0;
    size_t byte_pos = 0;
    const size_t n = m_text.size();
    if (byte_target > n) {
        byte_target = n;
    }

    while (byte_pos < byte_target) {
        unsigned char c = static_cast<unsigned char>(m_text[byte_pos]);
        size_t char_bytes = 1;
        size_t u16_adv = 1;
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
            u16_adv = 2;
        } else {
            char_bytes = 1;
            u16_adv = 1;
        }

        if (byte_pos + char_bytes > n) {
            char_bytes = n - byte_pos;
        }

        byte_pos += char_bytes;
        u16_cur += u16_adv;
    }
    return u16_cur;
}

bool TextBuffer::replaceUtf16(size_t u16_start, size_t u16_remove_len, const std::string& insert_utf8) {
    size_t byte_start = utf16ToByteIndex(u16_start);
    size_t byte_end = utf16ToByteIndex(u16_start + u16_remove_len);
    size_t byte_len = (byte_end >= byte_start) ? (byte_end - byte_start) : 0;
    return replace(byte_start, byte_len, insert_utf8);
}

bool TextBuffer::undo() {
    if (m_undo_stack.empty()) {
        return false;
    }
    EditOperation op = std::move(m_undo_stack.back());
    m_undo_stack.pop_back();

    size_t cur_len = op.new_text.length();
    if (op.byte_pos > m_text.size()) {
        op.byte_pos = m_text.size();
    }
    if (op.byte_pos + cur_len > m_text.size()) {
        cur_len = m_text.size() - op.byte_pos;
    }

    m_text.replace(op.byte_pos, cur_len, op.old_text);
    m_redo_stack.push_back(std::move(op));
    return true;
}

bool TextBuffer::redo() {
    if (m_redo_stack.empty()) {
        return false;
    }
    EditOperation op = std::move(m_redo_stack.back());
    m_redo_stack.pop_back();

    size_t cur_len = op.old_text.length();
    if (op.byte_pos > m_text.size()) {
        op.byte_pos = m_text.size();
    }
    if (op.byte_pos + cur_len > m_text.size()) {
        cur_len = m_text.size() - op.byte_pos;
    }

    m_text.replace(op.byte_pos, cur_len, op.new_text);
    m_undo_stack.push_back(std::move(op));
    return true;
}

bool TextBuffer::canUndo() const {
    return !m_undo_stack.empty();
}

bool TextBuffer::canRedo() const {
    return !m_redo_stack.empty();
}
