#define wait_time 900//ms

void utf_del_uni(char *s){//カナの文字コードをLCD用に変換する関数
    byte i=0;
    byte j=0;
    while(s[i]!='\0'){
        if((byte)s[i]==0xEF){
            if((byte)s[i+1]==0xBE) s[i+2] += 0x40;
            i+=2;
        }
        s[j]=s[i];
        i++;
        j++;
    }
    s[j]='\0';
}

void wait_lcd_func(){
  if(wait_lcd){
    wait_lcd = false;
    unsigned int start_time = millis();
    while(true){
      if(start_time + wait_time < millis()){
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

