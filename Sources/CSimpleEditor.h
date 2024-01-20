#ifndef __CSIMPLEEDITOR__
#define __CSIMPLEEDITOR__
#include <vector>
#define COLOR_EDITOR_BACKGROUND M5.Lcd.color565(0, 0, 255)
#define COLOR_EDITOR_TEXT M5.Lcd.color565(200, 200, 200)
#define COLOR_EDITOR_CURSOR M5.Lcd.color565(90, 90, 90)
#define COLOR_EDITOR_STATUS M5.Lcd.color565(0, 255, 255)
#define COLOR_EDITOR_STATUS_TEXT M5.Lcd.color565(0, 0, 0)

class CSimpleEditor {
private:
  std::vector<String> m_vLines;

  int m_iFontWidth;
  int m_iFontHeight;

  int m_iCurrentLine;
  int m_iCursorX;
public:
  CSimpleEditor() {
    m_iCurrentLine = 0;
    m_iCursorX = 0;

    m_iFontWidth = 0;
    m_iFontHeight = 0;
  }
  void Clear() {
    m_vLines.clear();
    m_iCursorX = 0;
    m_iCurrentLine = 0;
  }
  int GetFontWidth() {
    return m_iFontWidth;
  }
  int GetFontHeight() {
    return m_iFontHeight;
  }
  int GetCursorPosition() {
    return m_iCursorX;
  }
  int GetCurrentLine() {
    return m_iCurrentLine;
  }
  int GetLinesCount() {
    return m_vLines.size();
  }
  std::vector<String> GetData() {
    return m_vLines;
  }
  bool Load(const String& rstrFileName) {
    Clear();
    File fFile = SPIFFS.open(rstrFileName, FILE_READ);
    if (!fFile) return false;
    while (fFile.available()) {
      String line = fFile.readStringUntil('\n');
      m_vLines.push_back(line);
    }
    fFile.close();
    return true;
  }
  bool Save(const String& rstrFileName) {
    File fFile = SPIFFS.open(rstrFileName, FILE_WRITE);
    if (!fFile) return false;
    for (int x = 0; x < m_vLines.size(); x++) {
      fFile.println(m_vLines[x]);
    }
    fFile.close();
    return true;
  }
  int GetSize() {
    size_t totalSize = 0;
    for (const String& line : m_vLines) {
      totalSize += line.length();
    }
    return totalSize;
  }
  void Set(const int& rm_iFontWidth, const int& rm_iFontHeight) {
    m_iFontWidth = rm_iFontWidth;
    m_iFontHeight = rm_iFontHeight;
  }
  void handleKeyInput(char key) {
    if (m_vLines.size() == 0)
      m_vLines.push_back("");

    if (key == ASCII_END) {
      m_iCursorX = m_vLines[m_iCurrentLine].length();
    }
    if (key == ASCII_HOME) {
      m_iCursorX = 0;
    }
    if (key == ASCII_ARROW_UP) {
      if (m_iCurrentLine > 0) {
        m_iCurrentLine--;

        // Check if cursorX at correct position
        if (m_iCursorX > m_vLines[m_iCurrentLine].length())
          m_iCursorX = m_vLines[m_iCurrentLine].length();
      }
    }
    if (key == ASCII_ARROW_DOWN) {
      if (m_iCurrentLine < m_vLines.size() - 1) {
        m_iCurrentLine++;

        // Check if cursorX at correct position
        if (m_iCursorX > m_vLines[m_iCurrentLine].length())
          m_iCursorX = m_vLines[m_iCurrentLine].length();
      }
    }
    if (key == ASCII_ARROW_LEFT) {
      if (m_iCursorX > 0)
        m_iCursorX--;
    }
    if (key == ASCII_ARROW_RIGHT) {
      if (m_iCursorX < m_vLines[m_iCurrentLine].length())
        m_iCursorX++;
    }
    // Handle Backspace
    if (key == ASCII_BACKSPACE) {
      if (m_iCursorX > 0) {
        m_vLines[m_iCurrentLine].remove(m_iCursorX - 1, 1);
        m_iCursorX--;
      } else if (m_iCurrentLine > 0) {
        // Cursor is at the start of a line that is not the first line
        int len = m_vLines[m_iCurrentLine - 1].length();
        // Merge the current line with the previous one
        m_vLines[m_iCurrentLine - 1] += m_vLines[m_iCurrentLine];
        // Remove the current line
        m_vLines.erase(m_vLines.begin() + m_iCurrentLine);
        m_iCurrentLine--;
        m_iCursorX = len;  // Set cursor at the end of the now-merged line
      }
    }
    // Handle New Line
    else if (key == ASCII_ENTER) {
      // Split the current line at the cursor position
      String currentLine = m_vLines[m_iCurrentLine];
      String lineBeforeCursor = currentLine.substring(0, m_iCursorX);
      String lineAfterCursor = currentLine.substring(m_iCursorX);

      // Update the current line with the text before the cursor
      m_vLines[m_iCurrentLine] = lineBeforeCursor;

      // Insert a new line after the current line with the text after the cursor
      m_vLines.insert(m_vLines.begin() + m_iCurrentLine + 1, lineAfterCursor);

      // Move to the new line and reset the cursor position
      m_iCurrentLine++;
      m_iCursorX = 0;
    }
    // Handle Regular Character
    else if (key >= 0x20 && key < 0x7F) {
      String strChar = String(key);
      strChar.toUpperCase();
      // Insert the character at the current cursor position
      m_vLines[m_iCurrentLine] = m_vLines[m_iCurrentLine].substring(0, m_iCursorX) + strChar + m_vLines[m_iCurrentLine].substring(m_iCursorX);
      // Move the cursor one position to the right
      m_iCursorX++;
    }
  }
};

#endif