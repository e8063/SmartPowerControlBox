void all_on(){
  stop_xbee=true;
  SSerial.write('1');//all on signal 1
  succsessfully();
}

void pattern(){
  stop_xbee=true;
  SSerial.write('P');//pattern signal P
  succsessfully();
}

void all_off(){
  stop_xbee=false;
  SSerial.write('0');//pattern signal 0
  succsessfully();
}
