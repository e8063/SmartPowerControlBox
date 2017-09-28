#define voltageConst 1.67
#define NOISE_WATT_THRESHOLD 0.5

double read_current(){//電圧から電流を計算する関数
  int data = analogRead(current_vol);
  double voltage;
  voltage = (double)data/1024.0;//0.0~1.0
  return voltage*36.7-36.7/2.0;//peak
  //return voltage*73.3-73.3/2.0;//電流センサが30Aの場合はこれを使う
}

double read_voltage(){//分圧により1/2に減圧
  int data = analogRead(voltage_vol);
  return (100.0-(data/1024.0)*200.0)*voltageConst;//peak
}

int secCount;
double voltSqSec, currentSqSec;          // V^2, A^2 の1秒積算値
double voltCurrentSec;                   // V * A の1秒積算値

void resetSecValues(){
  secCount = 0;
  voltSqSec = currentSqSec = voltCurrentSec = 0.0;
}

void sample(){
  double current = read_current();
  double voltage = read_voltage();
  secCount++;

  voltSqSec      += voltage * voltage;
  currentSqSec   += current * current;
  voltCurrentSec += voltage * current;

  if (secCount == SAMPLE_PER_SEC) {
    now_voltage = sqrt(voltSqSec / SAMPLE_PER_SEC);
    now_current = sqrt(currentSqSec / SAMPLE_PER_SEC);
    now_power = voltCurrentSec / SAMPLE_PER_SEC;
    va = now_voltage * now_current;
    powerFactor = now_power / va;

    if (now_power < 0) { // ノイズ or 配線間違い (電圧と電流が逆位相) のとき
      now_power = -now_power;
      powerFactor = -powerFactor;
    }
    if (now_voltage < 10)
      now_voltage = 0.0;
      
    if (now_power < NOISE_WATT_THRESHOLD) { //ノイズ
      now_current = 0;
      now_power = 0;
      va = 0;
      powerFactor = 0;
    }
    //total_power = total_power + (unsigned long)(now_power*1.0/*測定周期[s]*/ + 0.5/*四捨五入*/);
    resetSecValues();
  }
}

void power_func(){
  if(power_send){
    char senddata[20];
    char char_buf[10];
    if(now_voltage < 10.0)
      senddata[0] = 'B';//breakertripped
    else if(now_voltage < 80.0)
      senddata[0] = 'L';//Low voltage
    else
      senddata[0] = 'F';//fine
    senddata[1] = '\0';
    dtostrf(now_power,-1,2,char_buf);
    strcat(senddata,char_buf);
    XBee.sendMesseage(2,senddata);
  }  
}

bool read_power_send(){
  if(EEPROM.read(powersend_setting_address) == 0)
    return false;
  return true;
}

void write_power_send(bool value){
  EEPROM.write(powersend_setting_address,value);
  lcd_backlight_eco = value;
}
