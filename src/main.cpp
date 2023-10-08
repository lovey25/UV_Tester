
/***************************************************************************************************
  [UV sensor]
    Level - 3.3v
    Signal - A0
  [LCD]
    SDA - A4
    SCL - A5
***************************************************************************************************/

#pragma GCC optimize("O3") // code optimisation controls - "O2" & "O3" code performance, "Os" code size
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ezButton.h>

#define COLUMS 16             // LCD columns
#define ROWS 2                // LCD rows
#define LCD_SPACE_SYMBOL 0x20 // space symbol from LCD ROM, see p.9 of GDM2004D datasheet

LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01, 4, 5, 6, 16, 11, 12, 13, 14, POSITIVE);

ezButton btn(2);                  // ezButton 객체를 D7 핀에 연결;
const int SHORT_LONG_TIME = 1000; // milliseconds
unsigned long pressedTime = 0;
unsigned long releasedTime = 0;
bool isPressing = false;
bool isLongDetected = false;

uint8_t mode = 0; // 동작 모드 - 0: 대기, 1: UV 측정, 2: 차단률 측정
float denominator = 1;
float numerator = 1;
bool isCalculate = false;
uint16_t UV = 0;       // 정수변수 선언 (범위: 0~65535)
float vVal = 0.0;      // 실수변수 선언
float blockRate = 0.0; // 차단률을 저장할 실수변수

void initMode()
{
  lcd.clear();

  switch (mode)
  {
  case 0:
    lcd.setCursor(0, 0);
    lcd.print(F("EveryX"));
    lcd.setCursor(0, 1);
    lcd.print(F("EveryUV-Tester"));
    break;
  case 1:
    lcd.setCursor(0, 0);
    lcd.print(F("UV meter"));
    break;
  case 2:
    lcd.setCursor(0, 0);
    lcd.print(F("UV block rate"));
    lcd.setCursor(0, 1);
    lcd.print(F("000.0% / 0.00V"));
    break;
  default:
    break;
  }
}

void setup()
{
  Serial.begin(9600);

  btn.setDebounceTime(50); // debounce time

  while (lcd.begin(COLUMS, ROWS, LCD_5x8DOTS) != 1) // colums, rows, characters size
  {
    Serial.println(F("PCF8574 is not connected or lcd pins declaration is wrong. Only pins numbers: 4,5,6,16,11,12,13,14 are legal."));
    delay(5000);
  }

  lcd.print(F("PCF8574 is OK...")); //(F()) saves string to flash & keeps dynamic memory free
  delay(500);

  initMode();
}

void changeMode()
{
  mode += 1;
  if (mode == 3)
    mode = 0;

  initMode();
}

void shortClick()
{
  if (mode == 2)
    isCalculate = !isCalculate;
}

void subloop()
{
  // lcd.write(LCD_SPACE_SYMBOL); //"write()" is faster than "lcd.print()"
  switch (mode)
  {
  case 0:
    break;
  case 1:
    lcd.setCursor(9, 0);
    UV = analogRead(A0);                       // A0 포트값을 0~1023사이의 값으로 데이터 수집
    vVal = UV * (5.0 / 1023.0);                // 전압값으로 변환
    lcd.print(vVal);                           // 측정값 시리얼 모니터로 출력
    lcd.printHorizontalGraph('>', 1, UV, 675); // bar name, 1-st  row, current value, maximum value
    break;

  case 2:
  {
    if (isCalculate)
    {
      numerator = analogRead(A0) * (5.0 / 1023.0);
      blockRate = (1-numerator / denominator) * 100;
      char temp[6];
      dtostrf(blockRate, 5, 1, temp);
      lcd.setCursor(0, 1);
      lcd.print(temp);
    }
    else
    {
      UV = analogRead(A0);
      denominator = UV * (5.0 / 1023.0);
      lcd.setCursor(9, 1);
      lcd.print(denominator);
    }
    break;
  }

  default:
    break;
  }
}

void loop()
{
  btn.loop(); // MUST call the loop() function first

  if (btn.isReleased()) // 버튼을 누르면
  {
    pressedTime = millis(); // 버튼을 누른 시간을 변수에 저장
    isPressing = true;
    isLongDetected = false;
  }

  if (btn.isPressed()) // 버튼을 놓았을 때
  {
    isPressing = false;
    releasedTime = millis(); // 버튼을 놓은 시간을 변수에 저장하고

    long pressDuration = releasedTime - pressedTime; // 버튼을 누르고 있었던 시간을 계산해서

    if (pressDuration < SHORT_LONG_TIME) // 길게 누르기 판단 기준보다 짧으면
    {
      // UV 차단률 측정 - 한번 누르면 최대값 저장하고 다시 누르면 최대값 대비 백분률 표시
      Serial.println("!!!");
      shortClick();
    }
  }

  if (isPressing == true && isLongDetected == false) // 버튼이 눌려져 있고, 롱클릭 상태가 아니면
  {
    long pressDuration = millis() - pressedTime; // 얼마나 길게 눌려지고 있는지 계산해서

    if (pressDuration > SHORT_LONG_TIME) // 길게 누르기 판단 기준보다 길면
    {
      isLongDetected = true;
      changeMode();
      Serial.println("changemode");
    }
  }

  subloop();
}
