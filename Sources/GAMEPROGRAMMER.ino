// TODO: Change CBasicInterpreter to CEngine or something

#include <M5Stack.h>
#include <M5Display.h>
#include <M5Faces.h>
#include "Free_Fonts.h"
#include "SPIFFS.h"
#include "AsciiCodes.h"
#include "CSimpleEditor.h"
#include "CBasicInterpreter.h"
#define TEXTURE_ATLAS_WIDTH 128
#define TEXTURE_ATLAS_HEIGHT 128
#define KEYBOARD_I2C_ADDR 0X08
#define KEYBOARD_INT 5
#define ACTION_BUTTON_PRESSED_TIME 2000

CSimpleEditor m_SimpleEditor;
TFT_eSprite img = TFT_eSprite(&M5.Lcd);

enum eStates {
  STATE_START,
  STATE_CODE,
  STATE_TEXTURES,
  STATE_PLAY
} m_eState;

void setup() {
  m_eState = eStates::STATE_START;


  M5.begin();
  M5.Power.begin();
  M5.Speaker.end();
  M5.Lcd.fillScreen(TFT_BLACK);

  img.setColorDepth(8);
  img.createSprite(M5.Lcd.width(), M5.Lcd.height());
  img.fillSprite(TFT_BLACK);

  img.setTextColor(WHITE);
  img.setFreeFont(FMBO18);
  img.setTextDatum(CC_DATUM);
  img.drawString("KOPICO", (int)(img.width() / 2), (int)(img.height() / 2), 1);
  img.drawString("loading, please wait", (int)(img.width() / 2), (int)(img.height() / 2) + 20, 2);
  img.pushSprite(0, 0);
  delay(2000);

  while (!SPIFFS.begin()) {
    SPIFFS.format();
    Serial.println("Failed to mount file system");
    delay(1000);
  }

  img.fillSprite(TFT_BLACK);
  img.pushSprite(0, 0);
  img.setFreeFont(FMB9);

  // Sets our text
  m_SimpleEditor.Set(img.textWidth("M"), 16);

  // Sets our pico engine
  CBasicInterpreter::GetInstance().Setup(img);
}


void loop() {

  CBasicInterpreter::GetInstance().Tick();

  switch (m_eState) {
    case eStates::STATE_START:
      {

        img.fillSprite(TFT_BLACK);
        img.setTextDatum(CC_DATUM);
        img.setTextColor(WHITE);
        img.setFreeFont(FMBO18);
        img.drawString("KOPICO", (int)(img.width() / 2), (int)(img.height() / 2), 1);
        img.drawString("what next?", (int)(img.width() / 2), (int)(img.height() / 2) + 20, 2);

        img.drawString("texture", (int)(img.width() / 2) - 100, (int)(img.height() - 10), 2);
        img.drawString("code", (int)(img.width() / 2), (int)(img.height() - 10), 2);
        img.drawString("flash", (int)(img.width() / 2) + 100, (int)(img.height() - 10), 2);


        if (M5.BtnB.isPressed()) {
          m_SimpleEditor.Load("/TEST.BAS");
          m_eState = eStates::STATE_CODE;
        }
      }
      break;

    case eStates::STATE_CODE:
      {
        // Back to main menu
        if (M5.BtnA.pressedFor(ACTION_BUTTON_PRESSED_TIME)) {
          m_SimpleEditor.Clear();
          m_eState = eStates::STATE_START;
        }

        // Keep holding to start
        if (M5.BtnB.pressedFor(ACTION_BUTTON_PRESSED_TIME)) {
          img.fillSprite(TFT_BLACK);
          img.setTextColor(TFT_WHITE);
          img.setCursor(0, 0);
          img.println("");
          img.print("Running ");
          CBasicInterpreter::GetInstance().RunScript("/TEST.BAS");
        }
        if (M5.BtnC.pressedFor(ACTION_BUTTON_PRESSED_TIME)) {
          // This opens up save / load functinality

          // Save load
          img.fillSprite(TFT_BLACK);
          img.setTextColor(TFT_WHITE);
          img.setCursor(0, 0);
          img.println("");
          img.print("Saving ");
          bool bSave = m_SimpleEditor.Save("/TEST.BAS");
          if (bSave) img.println("OK");
          else img.println("FAILED");
          img.pushSprite(0, 0);
          delay(1000);
        }

        // Pass chars to editor
        uint8_t uiFaceChar = CBasicInterpreter::GetInstance().BAS_FaceKey();
        if (uiFaceChar != 0) {
          m_SimpleEditor.handleKeyInput(uiFaceChar);
          Serial.printf("%x\n", uiFaceChar);
        }

        //
        // TODO - Move this to CSimpleEditor::Render(TFT_eSprite&)
        //
        img.fillSprite(COLOR_EDITOR_BACKGROUND);
        img.setFreeFont(FMB9);
        img.setTextColor(COLOR_EDITOR_TEXT);

        // Draw last x lines
        std::vector<String> vLines = m_SimpleEditor.GetData();
        int iLinesPerScreen = 24;
        int iCharsPerLine = 29;
        int firstLine = max(0, m_SimpleEditor.GetCurrentLine() - iLinesPerScreen / 2);

        // Adjust firstChar calculation for proper horizontal scrolling
        int cursorPosition = m_SimpleEditor.GetCursorPosition();
        int firstChar;
        if (cursorPosition >= iCharsPerLine) {
          firstChar = max(0, cursorPosition - (iCharsPerLine - 1));
        } else {
          firstChar = 0;
        }

        // Draw the cursor
        if ((millis() / 500) % 2) {
          int cursorX = (m_SimpleEditor.GetCursorPosition() - firstChar) * m_SimpleEditor.GetFontWidth();
          int cursorY = m_SimpleEditor.GetFontWidth() / 4 + (m_SimpleEditor.GetCurrentLine() - firstLine) * m_SimpleEditor.GetFontHeight();
          int cursorWidth = m_SimpleEditor.GetFontWidth();
          int cursorHeight = m_SimpleEditor.GetFontHeight();

          // Ensure the cursor is within the visible screen area
          if (m_SimpleEditor.GetCursorPosition() >= firstChar && m_SimpleEditor.GetCursorPosition() < firstChar + iCharsPerLine && m_SimpleEditor.GetCurrentLine() >= firstLine && m_SimpleEditor.GetCurrentLine() < firstLine + iLinesPerScreen) {
            img.fillRect(cursorX, cursorY, cursorWidth, cursorHeight, COLOR_EDITOR_CURSOR);
          }
        }

        for (int line = firstLine; line < min((int)vLines.size(), firstLine + iLinesPerScreen); line++) {
          for (int character = firstChar; character < min((int)vLines[line].length(), firstChar + iCharsPerLine); character++) {

            // Adjust cursor position for scrolling
            int displayX = (character - firstChar) * m_SimpleEditor.GetFontWidth();
            int displayY = m_SimpleEditor.GetFontHeight() + (line - firstLine) * m_SimpleEditor.GetFontHeight();
            img.setCursor(displayX, displayY);
            img.print(vLines[line][character]);
          }
        }

        // Render the status bar
        img.fillRect(0, M5.Lcd.height() - m_SimpleEditor.GetFontHeight(), M5.Lcd.width(), m_SimpleEditor.GetFontHeight(), COLOR_EDITOR_STATUS);
        img.setTextColor(COLOR_EDITOR_STATUS_TEXT);

        if (M5.BtnA.isPressed()) {
          img.drawString("HOLD to exit to main menu", 0, M5.Lcd.height() - m_SimpleEditor.GetFontHeight() / 2);
        } else if (M5.BtnB.isPressed()) {
          img.drawString("HOLD to run the program", 0, M5.Lcd.height() - m_SimpleEditor.GetFontHeight() / 2);
        } else if (M5.BtnC.isPressed()) {
          img.drawString("HOLD to save or load", 0, M5.Lcd.height() - m_SimpleEditor.GetFontHeight() / 2);
        } else
          img.drawString(String(m_SimpleEditor.GetCurrentLine()) + "/" + String(m_SimpleEditor.GetLinesCount()) + "/" + String(m_SimpleEditor.GetCursorPosition()) + " " + String(m_SimpleEditor.GetSize()) + "b " + String(esp_get_free_heap_size()) + "b R:" + String(CBasicInterpreter::GetInstance().GetLastReturnCode()) + " " + String(M5.Power.getBatteryLevel()) + "%", 0, M5.Lcd.height() - m_SimpleEditor.GetFontHeight() / 2);
      }
      break;
  }

  img.pushSprite(0, 0);
}
