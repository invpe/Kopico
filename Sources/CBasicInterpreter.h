#ifndef __CBASICINTERPRETER__
#define __CBASICINTERPRETER__
#include <M5Stack.h>
#include <M5Display.h>
#include <M5Faces.h>
#include "my_basic.h"
class CBasicInterpreter {

public:
  static CBasicInterpreter& GetInstance();
  void Setup(TFT_eSprite& pSprite);
  int GetLastReturnCode();
  static int MBStep(struct mb_interpreter_t* s, void** l, const char* f, int p, unsigned short row, unsigned short col);
  int RunScript(const String& rstrBASFile);
  ~CBasicInterpreter();

  //
  uint16_t rread16(fs::File& f);
  uint32_t rread32(fs::File& f);
  bool BMPLoad(const String& rstrFile);
  void DrawBMP(uint16_t startX, uint16_t startY, uint16_t endX, uint16_t endY, int screenX, int screenY);
  int GetTextureWidth();
  int GetTextureHeight();
  void Tick();
  // Exposed
  void BAS_ClearScreen(const int& iColor);
  void BAS_Render();
  void BAS_SetPixel(const int& iX, const int& iY, const int& iColor);
  void BAS_SetCursor(const int& iX, const int& iY);
  void BAS_Print(const String& strText);
  int BAS_FaceKey();
private:
  CBasicInterpreter();
  int m_iLastRetCode;
  int m_iTextureWidth;
  int m_iTextureHeight;
  TFT_eSprite* m_pSpritePointer;
  uint8_t* m_pTexture = nullptr;

  M5Faces M5Face;
};

static int _clearscreen(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;
  int_t iColor = 0;
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_int(s, l, &iColor));
  mb_check(mb_attempt_close_bracket(s, l));
  CBasicInterpreter::GetInstance().BAS_ClearScreen(iColor);
  mb_check(mb_push_int(s, l, 0));
  return result;
}
static int _render(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));
  CBasicInterpreter::GetInstance().BAS_Render();
  mb_check(mb_push_int(s, l, 0));
  return result;
}
static int _setpixel(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;
  int_t iX, iY, iColor;
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_int(s, l, &iX));
  mb_check(mb_pop_int(s, l, &iY));
  mb_check(mb_pop_int(s, l, &iColor));
  mb_check(mb_attempt_close_bracket(s, l));
  CBasicInterpreter::GetInstance().BAS_SetPixel(iX, iY, iColor);
  mb_check(mb_push_int(s, l, 0));
  return result;
}
static int _sleep(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;
  int_t iMillis = 0;
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_int(s, l, &iMillis));
  mb_check(mb_attempt_close_bracket(s, l));
  delay(iMillis);
  mb_check(mb_push_int(s, l, 0));
  return result;
}
static int _text(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;
  mb_check(mb_attempt_open_bracket(s, l));
  char* m;
  mb_check(mb_pop_string(s, l, &m));
  mb_check(mb_attempt_close_bracket(s, l));
  CBasicInterpreter::GetInstance().BAS_Print(String(m));
  mb_check(mb_push_int(s, l, 0));
  return result;
}
static int _texture(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;
  mb_check(mb_attempt_open_bracket(s, l));
  char* m;
  mb_check(mb_pop_string(s, l, &m));
  mb_check(mb_attempt_close_bracket(s, l));
  int iRes = CBasicInterpreter::GetInstance().BMPLoad(String(m));
  mb_check(mb_push_int(s, l, iRes));
  return result;
}
static int _setcursor(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;
  int_t iX, iY;
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_int(s, l, &iX));
  mb_check(mb_pop_int(s, l, &iY));
  mb_check(mb_attempt_close_bracket(s, l));
  CBasicInterpreter::GetInstance().BAS_SetCursor(iX, iY);
  mb_check(mb_push_int(s, l, 0));
  return result;
}
static int _texel(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;
  int_t iStartX, iStartY;
  int_t iEndX, iEndY;
  int_t iPosX, iPosY;

  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_pop_int(s, l, &iStartX));
  mb_check(mb_pop_int(s, l, &iStartY));
  mb_check(mb_pop_int(s, l, &iEndX));
  mb_check(mb_pop_int(s, l, &iEndY));
  mb_check(mb_pop_int(s, l, &iPosX));
  mb_check(mb_pop_int(s, l, &iPosY));
  mb_check(mb_attempt_close_bracket(s, l));
  CBasicInterpreter::GetInstance().DrawBMP(iStartX, iStartY, iEndX, iEndY, iPosX, iPosY);
  mb_check(mb_push_int(s, l, 0));
  return result;
}
static int _millis(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));
  mb_check(mb_push_int(s, l, static_cast<int>(millis())));
  return result;
}
static int _seconds(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));
  mb_check(mb_push_int(s, l, static_cast<int>(millis() / 1000)));
  return result;
}
static int _fkey(struct mb_interpreter_t* s, void** l) {
  int result = MB_FUNC_OK;
  mb_check(mb_attempt_open_bracket(s, l));
  mb_check(mb_attempt_close_bracket(s, l));
  mb_check(mb_push_int(s, l, CBasicInterpreter::GetInstance().BAS_FaceKey()));
  return result;
}
#endif