#define wait_time 700//ms

void wait_lcd_func(){
  if(wait_lcd){
    wait_lcd = false;
    unsigned long start_time = millis();
    while(true){
      if(start_time + wait_time <= millis()){
        break;
      }
      if(wait_lcd){
        start_time = millis();//追加でwaitが入った場合に待機時間を延長
        wait_lcd = false;
      }
      delay(100);
    }
  }
}

void lcd_clear(){//トリガーdelayを実装したclear関数
  wait_lcd_func();
  lcd.clear();
}

void lcd_print(const char *data){//フラッシュメモリ上の文字を表示するための関数
  char string[100];
  strcpy_P(string,data);
  lcd.print(string);
}

void back_light_off(){//バックライトの消灯を行う関数
  digitalWrite(lcd_backlight,LOW);
  while(true){
    if(read_digi(button_select)||read_digi(button_enter)||read_digi(button_return))
      break;
  }
  digitalWrite(lcd_backlight,HIGH);
  standby3(button_enter,button_select,button_return);
}

bool read_backlight_eco(){
  if(EEPROM.read(backlight_setting_address) == 0)
    return false;
  return true;
}

void write_backlight_eco(bool value){
  EEPROM.write(backlight_setting_address,value);
  lcd_backlight_eco = value;
}

