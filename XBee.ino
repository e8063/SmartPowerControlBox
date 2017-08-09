void xbee_loop(){//シリアル受信・送信を行う関数
  
  if(!stop_xbee){
  XBee.loopAction();
  char *recieve = XBee.recieveRXData(1);
  
  if(recieve){//モード転送
    SSerial.write(recieve);
    lcd.clear();
    lcd.print("Recieved:");
    lcd.print("0x");
    lcd.print(recieve[0],16);
    wait_lcd = true;
  }
  //wdt_reset();//ウォッチドッグタイマをリセット
  
  static bool led_flag = false;
  led_flag = !led_flag;
  digitalWrite(13,led_flag);//割り込み終了時に13番ピンのステータスを反転
  }
  delay(50);
}
