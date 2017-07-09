void blinks(int on_time,int off_time,int trigger){//trigerを持つ点滅を行う関数
  while(true){
    SSerial.write('1');
    if(trigger_delay(on_time,trigger))
      break;
    SSerial.write('0');
    if(trigger_delay(off_time,trigger))
      break;
  }
  SSerial.write('0');
}

bool trigger_delay(long ms,int trigger){//pattern_modeを停止トリガーとするdelay関数
  unsigned long start_time = millis();
  while(true){
    delay(10);
    if(start_time + ms <= millis())
      break;
    if(pattern_mode != trigger)
      return true;
  }
  return false;
}

void all_on(){
  pattern_mode = 3;
  SSerial.write('1');//all on signal 1
  succsessfully();
  exit_return();
  SSerial.write('0');
  pattern_mode = 0;
}

void pattern(){
  pattern_mode = 3;
  SSerial.write('P');//pattern signal P
  succsessfully();
  exit_return();
  SSerial.write('0');
  pattern_mode = 0;
}


void pattern_loop(){//モジュールによる点灯制御を行う関数
  switch(pattern_mode){
          
    case 1://lcd_blinks triger:1;
      blinks(1000,800,11);
      break;
    
    case 2://xbee_blinks triger:2;
      blinks(xbee_on,xbee_off,2);
      break;

    default:
    case 0:
      delay(80);
      break;
  }
}
