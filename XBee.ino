int xbee_on,xbee_off;

void xbee_loop(){//シリアル受信・送信を行う関数
  XBee.loopAction();
  char *recieve = XBee.recieveRXData(1);
  
  if(recieve == NULL){
    //Do nothing
  }
  
  else if(recieve[0] != 127){//モード転送
    SSerial.write(recieve);
    lcd.clear();
    lcd.print("RecieveMassage!");
    lcd.setCursor(8, 1);
    lcd.print("0x");
    lcd.print(recieve[0],16);
    wait_lcd = true;
  }
  else{
    lcd.clear();
    lcd.print("RecieveMassage!");
    lcd.setCursor(8, 1);
    lcd.print("Blinks");
    xbee_on = recieve[1]*100;
    xbee_off = recieve[2]*100;
    blinks(xbee_on,xbee_off,2);
  }
  //wdt_reset();//ウォッチドッグタイマをリセット
  
  static bool led_flag = false;
  led_flag = !led_flag;
  digitalWrite(13,led_flag);//割り込み終了時に13番ピンのステータスを反転
  delay(80);
}
