// #define DISPLAY_ON //ON

//#ifdef DISPLAY_ON
#include <LiquidCrystal_I2C.h>
// set the LCD number of columns and rows
#define LCD_COLUMNS 20
#define LCD_ROWS 4
int fila = 0;
// set LCD address, number of columns and rows
// if you don't know your display address, run an I2C scanner sketch
LiquidCrystal_I2C lcd(0x27, LCD_COLUMNS, LCD_ROWS);

//#endif

// Prototype
void lcdStart();
void lcdText(int column = 0, int row = 0, String text = "");

void lcdStart()
{
    lcd.init();
    // turn on LCD backlight
    lcd.backlight(); 

    lcd.setCursor(0, 0);
    // print message
    //lcd.print("LCD OK!");
    lcdText(0,0,"LCD OK!");
    wait(2000);
    lcd.clear();
    _APP_DEBUG_(F("LCD INIT"), F(LCD_COLUMNS + "x" + LCD_ROWS));
}

void lcdText(int column, int row,  String text){
    lcd.setCursor( column, row );
    lcd.print(text);
}