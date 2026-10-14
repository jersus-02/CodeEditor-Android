#ifndef TEXT_BUFFER_H
#define TEXT_BUFFER_H

#include <string>
#include <vector>
#include <cstddef>

struct EditOperation {
    size_t byte_pos;
    std::string old_text;
    std::string new_text;
};

class TextBuffer {
public:
    TextBuffer();
    explicit TextBuffer(const std::string& initial_text);
    ~TextBuffer() = default;

    void setText(const std::string& text);
    const std::string& getText() const;

    bool insert(size_t pos, const std::string& str);
    bool erase(size_t pos, size_t len);
    bool replace(size_t pos, size_t len, const std::string& str);

    // Replace operation specified in UTF-16 code unit indices (matching Java String indexing)
    bool replaceUtf16(size_t u16_start, size_t u16_remove_len, const std::string& insert_utf8);

    size_t length() const;
    int lineCount() const;

    bool undo();
    bool redo();

    bool canUndo() const;
    bool canRedo() const;

    size_t utf16ToByteIndex(size_t u16_index) const;
    size_t byteToUtf16Index(size_t byte_index) const;

private:
    std::string m_text;
    std::vector<EditOperation> m_undo_stack;
    std::vector<EditOperation> m_redo_stack;
};

#endif // TEXT_BUFFER_H
