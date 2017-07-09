#define voltageConst 1.18

double read_current(){//電圧から電流を計算する関数
  int data = analogRead(current_vol);
  double voltage;
  voltage = (double)data/1024.0;//0.0~1.0
  return voltage*73.3-73.3/2.0;//peak
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

    if (now_power < NOISE_WATT_THRESHOLD) { //ノイズ
      now_current = 0;
      now_power = 0;
      va = 0;
      powerFactor = 0;
    }
    total_power = total_power + (unsigned long)(now_power*1.0/*測定周期[s]*/ + 0.5/*四捨五入*/);
    resetSecValues();
  }
}