#include "CBasicInterpreter.h"
CBasicInterpreter::CBasicInterpreter() {
  m_iLastRetCode = 0;
  m_pSpritePointer = nullptr;
  m_pTexture = nullptr;
  m_iTextureWidth = -1;
  m_iTextureHeight = -1;
}
CBasicInterpreter& CBasicInterpreter::GetInstance() {
  static CBasicInterpreter sInstance;
  return sInstance;
}
void CBasicInterpreter::Setup(TFT_eSprite& pSprite) {

  Wire.begin();
  //
  m_pSpritePointer = &pSprite;
}
int CBasicInterpreter::GetLastReturnCode() {
  return m_iLastRetCode;
}
int CBasicInterpreter::MBStep(struct mb_interpreter_t* s, void** l, const char* f, int p, unsigned short row, unsigned short col) {
  CBasicInterpreter::GetInstance().Tick();
  return MB_FUNC_OK;
}
int CBasicInterpreter::RunScript(const String& rstrBASFile) {

  if (m_pSpritePointer == nullptr) return -1;

  File fScriptFile = SPIFFS.open(rstrBASFile, "r");
  if (fScriptFile) {
    String strScript = fScriptFile.readStringUntil('\0');
    fScriptFile.close();

    void** l = NULL;
    struct mb_interpreter_t* bas = NULL;

    mb_init();
    mb_open(&bas);
    mb_register_func(bas, "CLS", _clearscreen);
    mb_register_func(bas, "RENDER", _render);
    mb_register_func(bas, "PIXEL", _setpixel);
    mb_register_func(bas, "SLEEP", _sleep);
    mb_register_func(bas, "CURSOR", _setcursor);
    mb_register_func(bas, "TEXT", _text);
    mb_register_func(bas, "TEXTURE", _texture);
    mb_register_func(bas, "TEXEL", _texel);
    mb_register_func(bas, "SECONDS", _seconds);
    mb_register_func(bas, "MILLIS", _millis);
    mb_register_func(bas, "FKEY", _fkey);

    mb_debug_set_stepped_handler(bas, CBasicInterpreter::MBStep, NULL);


    if (mb_load_string(bas, strScript.c_str(), true) == MB_FUNC_OK) {
      strScript.clear();
      m_iLastRetCode = mb_run(bas, true);
    }
    mb_close(&bas);
    mb_dispose();
    return m_iLastRetCode;
  }
  return -1;
}

uint16_t CBasicInterpreter::rread16(fs::File& f) {
  uint16_t result;
  ((uint8_t*)&result)[0] = f.read();  // LSB
  ((uint8_t*)&result)[1] = f.read();  // MSB
  return result;
}

uint32_t CBasicInterpreter::rread32(fs::File& f) {
  uint32_t result;
  ((uint8_t*)&result)[0] = f.read();  // LSB
  ((uint8_t*)&result)[1] = f.read();
  ((uint8_t*)&result)[2] = f.read();
  ((uint8_t*)&result)[3] = f.read();  // MSB
  return result;
}
bool CBasicInterpreter::BMPLoad(const String& rstrFile) {

  if (m_pTexture != nullptr) {
    delete[] m_pTexture;
    m_pTexture = nullptr;
  }

  Serial.println("Loading texture " + rstrFile);
  File bmpFS = SPIFFS.open(rstrFile, "r");

  if (!bmpFS) {
    Serial.println("File not found");
    return false;
  }

  if (rread16(bmpFS) != 0x4D42) {  // Check for BMP signature
    Serial.println("Not a BMP file");
    bmpFS.close();
    return false;
  }

  rread32(bmpFS);  // Ignore file size
  rread32(bmpFS);  // Ignore reserved fields
  uint32_t seekOffset = rread32(bmpFS);

  uint32_t dibHeaderSize = rread32(bmpFS);
  uint32_t w = rread32(bmpFS);        // Width as unsigned
  int32_t h_signed = rread32(bmpFS);  // Height as signed

  uint32_t h;
  bool topDownBitmap = false;
  if (h_signed < 0) {
    h = static_cast<uint32_t>(-h_signed);
    topDownBitmap = true;  // The bitmap is top-down
  } else {
    h = static_cast<uint32_t>(h_signed);
  }

  if (rread16(bmpFS) != 1 || rread16(bmpFS) != 24 || rread32(bmpFS) != 0) {
    Serial.println("Unsupported BMP format");
    bmpFS.close();
    return false;
  }

  bmpFS.seek(seekOffset);

  // Calculate the size of the bitmap
  uint16_t padding = (4 - ((w * 3) & 3)) & 3;
  uint32_t bitmapSize = h * (w * 3 + padding);

  m_pTexture = new uint8_t[bitmapSize];
  if (!m_pTexture) {
    Serial.println("Failed to allocate memory for bitmap");
    bmpFS.close();
    return false;
  }

  // Read the bitmap into the allocated buffer
  bmpFS.read(m_pTexture, bitmapSize);
  bmpFS.close();
  m_iTextureWidth = w;
  m_iTextureHeight = h;

  Serial.printf("Texture loaded %ix%i\n", w, h);
  return true;
}
void CBasicInterpreter::DrawBMP(uint16_t startX, uint16_t startY, uint16_t endX, uint16_t endY, int screenX, int screenY) {

  if (m_pTexture == nullptr) return;  // Check if the bitmap is loaded

  
  bool topDown = false;

  const uint8_t transparentR = 0;
  const uint8_t transparentG = 255;
  const uint8_t transparentB = 255;

  // Clamping the coordinates to the size of the BMP
  startX = min(startX, m_iTextureWidth - 1);
  startY = min(startY, m_iTextureHeight - 1);
  endX = min(endX, m_iTextureWidth - 1);
  endY = min(endY, m_iTextureHeight - 1);

  for (uint32_t y = startY; y <= endY; ++y) {
    for (uint32_t x = startX; x <= endX; ++x) {
      // Calculate the position in the bitmap array
      uint32_t pos = (topDown ? y : (m_iTextureHeight - 1 - y)) * m_iTextureWidth * 3 + x * 3;

      // Extract RGB values
      uint8_t r = m_pTexture[pos + 2];
      uint8_t g = m_pTexture[pos + 1];
      uint8_t b = m_pTexture[pos];

      // Check if the color is the transparent one
      if (r == transparentR && g == transparentG && b == transparentB) {
        continue;  // Skip this pixel
      }

      // Convert to color format expected by img.drawPixel (usually 5-6-5 format for RGB)
      uint16_t color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);

      // Draw the pixel at the offset position on the screen
      m_pSpritePointer->drawPixel(screenX + (x - startX), screenY + (y - startY), color);
    }
  }
}
void CBasicInterpreter::Tick() {
  M5.update();
}
int CBasicInterpreter::GetTextureWidth() {
  return m_iTextureWidth;
}
int CBasicInterpreter::GetTextureHeight() {
  return m_iTextureHeight;
}
void CBasicInterpreter::BAS_ClearScreen(const int& iColor) {
  m_pSpritePointer->fillSprite(iColor);
}
void CBasicInterpreter::BAS_SetPixel(const int& iX, const int& iY, const int& iColor) {
  m_pSpritePointer->drawPixel(iX, iY, iColor);
}
void CBasicInterpreter::BAS_Render() {
  m_pSpritePointer->pushSprite(0, 0);
}
void CBasicInterpreter::BAS_Print(const String& strText) {
  m_pSpritePointer->print(strText);
}
void CBasicInterpreter::BAS_SetCursor(const int& iX, const int& iY) {
  m_pSpritePointer->setCursor(iX, iY);
}
int CBasicInterpreter::BAS_FaceKey() {
  Tick();
  return M5Face.getch();
}
CBasicInterpreter::~CBasicInterpreter() {
}