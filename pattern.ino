void all_on(){
  pattern_mode = 0;
  stop_xbee=true;
  SSerial.write('1');//all on signal 1
  succsessfully();
}

void pattern(){
  pattern_mode = 0;
  stop_xbee=true;
  SSerial.write('P');//pattern signal P
  succsessfully();
}

void all_off(){
  pattern_mode = 0;
  stop_xbee=false;
  SSerial.write('0');//pattern signal 0
  succsessfully();
}
