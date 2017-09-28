#include<XBeeLibrary.h>
#include<SoftwareSerial.h>
#include<LiquidCrystal.h>
#include <KanaLiquidCrystal.h>
#include<EEPROM.h>
#include<avr/wdt.h>
#include <avr/pgmspace.h>
#include <Scheduler.h>
#include <TimerOne.h>
#include <MsTimer2.h>
XBeeLibrary XBee;

/*電力計測関係*/
#define SAMPLE_PER_SEC 600
#define SAMPLE_PERIOD_USEC (1000000 / SAMPLE_PER_SEC)

/*入出力関係*/
#define button_select A0//選択ボタン接続端子
#define button_enter A1//決定ボタン接続端子
#define button_return A2//戻るボタン接続端子
#define current_vol  A3//電流センサ接続端子
#define voltage_vol A4//電圧測定端子
#define refarence_vol A5//校正用電圧入力端子 2.5Vを入力

#define lcd_backlight 4//LCDのバックライト制御ラインを接続する端子
#define lcd_ontime 30000//LCDのバックライトの点灯時間[ms]
#define backlight_setting_address 0
#define powersend_setting_address 1

SoftwareSerial SSerial(6,5);//ソフトウェアシリアル

KanaLiquidCrystal lcd(7,8,9,10,11,12);//LCD

volatile double now_power = 0.0;
volatile double now_voltage = 0.0;
volatile double now_current = 0.0;
volatile double powerFactor = 0.0;
volatile double va = 0.0;

bool stop_xbee = false;
bool wait_lcd = false;
bool lcd_backlight_eco;
bool power_send;
bool home_flug = false;
unsigned long lcd_offtime = lcd_ontime + 3000;//30000;

//フラッシュメモリ上にカナ文字は格納(メモリ節約)
const char enter[] PROGMEM = "Enter:ｹｯﾃｲ";
const char testmode[] PROGMEM = "ﾃｽﾄﾓｰﾄﾞ";
const char selectmanagement[] PROGMEM = "Select:ｼｽﾃﾑｶﾝﾘ";
const char test1[] PROGMEM = ">Test 1(ｾﾞﾝﾃﾝﾄｳ)";
const char test2[] PROGMEM = ">Test 2(ﾊﾟﾀｰﾝ)";
const char test3[] PROGMEM = ">Test 3(ｼｮｳﾄｳ)";
const char management[] PROGMEM = "ﾃﾞﾝﾘｮｸﾓﾆﾀｰ";
const char setting[] PROGMEM = "LCDﾉｾｯﾃｲ";
const char selectsetting[] PROGMEM ="Select:LCDﾉｾｯﾃｲ";
const char powerstring[] PROGMEM = "ｼｮｳﾋﾃﾞﾝﾘｮｸ";
const char factorstring[] PROGMEM = ">ﾘｷﾘﾂ";
const char vastring[] PROGMEM = ">ﾋｿｳﾃﾞﾝﾘｮｸ";
const char alwayson[] PROGMEM ="Enter:ﾂﾈﾆﾃﾝﾄｳ";
const char ecomode[] PROGMEM ="Select:ｴｺﾓｰﾄﾞ";
const char lcdbacklight[] PROGMEM = "LCDﾊﾞｯｸﾗｲﾄ";
const char warning[] PROGMEM = "ｼﾞｮｳﾀｲ:ｹｲｺｸ!";
const char lowvoltage[] PROGMEM = "ﾃﾞﾝｱﾂｶﾞﾃｲｶｼﾃｲﾏｽ";
const char breakertripped[] PROGMEM = "ﾌﾞﾚｰｶｶﾞｵﾁﾃｲﾏｽ";
const char fine[] PROGMEM = "ｼｽﾃﾑﾊｾｲｼﾞｮｳﾃﾞｽ";
const char exitreturn[] PROGMEM = "ﾓﾄﾞﾙ:return";
const char currentstring[] PROGMEM = ">ﾃﾞﾝﾘｭｳ";
const char voltagestring[] PROGMEM = ">ﾃﾞﾝｱﾂ";
const char starttext1[] PROGMEM = "ﾏｲﾂﾞﾙｺｳｾﾝ";
const char starttext2[] PROGMEM = "4E ｿｳｿﾞｳｺｳｶﾞｸ";
const char powersetting[] PROGMEM = "ﾃﾞﾝﾘｮｸｶﾝﾘ";
const char powersend[] PROGMEM = "ﾃﾞﾝﾘｮｸｿｳｼﾝｷﾉｳ";
const char powersendstring[] PROGMEM = "Enter:ｿｳｼﾝｽﾙ";
const char powernotsendstring[] PROGMEM = "Select:ｿｳｼﾝｼﾅｲ";
const char selectpower[] PROGMEM = "Select:ﾃﾞﾝﾘｮｸｶﾝﾘ";

byte mode = 100;
/*
  0 is Power mode 
  -> 100 is Power mode(Non Standby)  
  1 is Test　Signal Select mode
  -> 10 all on 
  -> 11 blinks
  2 is configure mode 
*/

void setup(){
  XBee.setup(true);
  XBee.setAddress(0x0013A200, 0x40B401BD);//1 Coordinator_address
  XBee.setAddress(0x0013A200, 0x406CB620);//2 Powermonitor_address
  SSerial.begin(9600);
  
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  
  pinMode(lcd_backlight,OUTPUT);
  digitalWrite(lcd_backlight,HIGH);

  lcd.begin(16, 2);
  lcd.kanaOn();
  ADCSRA = ADCSRA & 0xf8;//A/Dコンバータの高速化
  ADCSRA = ADCSRA | 0x04;
  resetSecValues();
  
  lcd_backlight_eco = read_backlight_eco();
  power_send = read_power_send();
  
  Scheduler.startLoop(xbee_loop);
  //Scheduler.startLoop(power_loop);
  
  Timer1.initialize(SAMPLE_PERIOD_USEC);
  Timer1.attachInterrupt(sample);
  
  lcd_print(starttext1);

  lcd.setCursor(0, 1);
  lcd_print(starttext2);
  
  delay(2000);
  lcd.clear();
  lcd.print("Smart Power");

  lcd.setCursor(1, 1);
  lcd.print("Control Box");

  delay(2000);
  
  status_lcd();
  delay(2000);
  
  wdt_enable(WDTO_8S);//ウォッチドッグタイマーを有効化
  MsTimer2::set(3000,power_func);
  MsTimer2::start();
  
}

void loop(){//LCDの表示を制御
  bool print_flag;
  switch(mode){
    case 0:
      lcd_power();
      standby2(button_select,button_enter);
    case 100:
      home_flug = true;
      while(true){
        if(now_voltage <= 80.0)
          status_lcd();
        else
          lcd_power();
        if(read_digi(button_select)||read_digi(button_enter)){
          home_flug = false;
          mode = 1;
          break;
        }
        if((lcd_offtime <= millis())&&lcd_backlight_eco)
          back_light_off();
      }
      
      break;
      
    case 1:
      //print_flag = false;
      lcd.clear();
      lcd_print(testmode);
      lcd.setCursor(0, 1);
      lcd_print(enter);
      standby2(button_select,button_enter);
      while(true){
        lcd.clear();
        lcd_print(testmode);
        if(read_digi(button_select)){
          mode = 2;
          break;
        }
        if(read_digi(button_enter)){          
          mode = 10;
          break;
        }
        if(read_digi(button_return)){
          mode = 0;
          break;
        }
        //切り替え実装
        print_flag = !print_flag;
        if(print_flag){
          lcd.setCursor(0, 1);
          lcd_print(enter);
          lcd.print("    ");
        }
        else{
          lcd.setCursor(0, 1);
          lcd_print(selectmanagement);
        }
        delay(400);
      }
      break;

    case 10:
      lcd_test1();
      standby(button_enter);
      while(true){
        lcd_test1();
        if(read_digi(button_select)){
          mode = 11;
          break;
        }
        if(read_digi(button_enter)){
          all_on();
          mode = 0;
          break;
        }
        if(read_digi(button_return)){
          mode = 1;
          break;
        }
        delay(100);
      }
      break;

    case 11:
      lcd_test2();
      standby(button_select);
      while(true){
        lcd_test2();
        if(read_digi(button_select)){
          mode = 12;
          
          break;
        }
        if(read_digi(button_enter)){
          pattern();
          mode = 0;
          break;
        }
        if(read_digi(button_return)){
          mode = 0;
          break;
        }
        delay(100);
      }
      break;
      
    case 12:
      lcd_test3();
      standby(button_select);
      while(true){
        lcd_test3();
        if(read_digi(button_select)){
          mode = 2;
          break;
        }
        if(read_digi(button_enter)){
          all_off();
          mode = 0;
          break;
        }
        if(read_digi(button_return)){
          mode = 0;
          break;
        }
        delay(100);
      }
      break;
      
    case 2:
      print_flag = false;
      lcd.clear();
      lcd_print(management);
      lcd.setCursor(0, 1);
      lcd_print(enter);
      standby(button_select);
      while(true){
        lcd.clear();
        lcd_print(management);
        if(read_digi(button_select)){
          mode = 3;
          break;
        }
        if(read_digi(button_enter)){          
          mode = 20;
          break;
        }
        if(read_digi(button_return)){
          mode = 0;
          break;
        }
        //切り替え実装
        print_flag = !print_flag;
        if(print_flag){
          lcd.setCursor(0, 1);
          lcd_print(enter);
          lcd.print("    ");
        }
        else{
          lcd.setCursor(0, 1);
          lcd_print(selectsetting);
        }
        delay(400);
      }
      break;

    case 20:
      lcd_voltage();
      standby2(button_enter,button_select);
      while(true){
        lcd_voltage();
        if(read_digi(button_enter)||read_digi(button_select)){          
          mode = 21;
          break;
        }
        if(read_digi(button_return)){
          mode = 0;
          break;
        }
        delay(100);
      }
      break;

    case 21:
      lcd_current();
      standby2(button_enter,button_select);
      while(true){
        lcd_current();
        if(read_digi(button_enter)||read_digi(button_select)){          
          mode = 22;
          break;
        }
        if(read_digi(button_return)){
          mode = 0;
          break;
        }
        delay(100);
      }
      break;
    
    case 22:
      lcd_va();
      standby2(button_enter,button_select);
      while(true){
        lcd_va();
        if(read_digi(button_enter)||read_digi(button_select)){          
          mode = 23;
          break;
        }
        if(read_digi(button_return)){
          mode = 0;
          break;
        }
        delay(100);
      }
      break;
    
    case 23:
      lcd_factor();
      standby2(button_enter,button_select);
      while(true){
        lcd_factor();
        if(read_digi(button_enter)||read_digi(button_select)){          
          mode = 24;
          break;
        }
        if(read_digi(button_return)){
          mode = 0;
          break;
        }
        delay(100);
      }
      break;


    case 3:
      print_flag = false;
      lcd.clear();
      lcd_print(setting);
      lcd.setCursor(0, 1);
      lcd_print(enter);
      standby(button_select);
      while(true){
        lcd.clear();
        lcd_print(setting);
        if(read_digi(button_select)){
          mode = 4;
          break;
        }
        if(read_digi(button_enter)){          
          mode = 30;
          break;
        }
        if(read_digi(button_return)){
          mode = 0;
          break;
        }
        //切り替え実装
        print_flag = !print_flag;
        if(print_flag){
          lcd.setCursor(0, 1);
          lcd_print(enter);
        }
        else{
          lcd.setCursor(0, 1);
          lcd_print(selectpower);
        }
        delay(400);
      }
      
      break;

    case 30:
      print_flag = false;
      lcd.clear();
      lcd_print(lcdbacklight);
      lcd.setCursor(0, 1);
      lcd_print(alwayson);
      standby(button_enter);
      while(true){
        lcd.clear();
        lcd_print(lcdbacklight);
        if(read_digi(button_enter)){      
          mode = 0;
          write_backlight_eco(false);
          lcd_backlight_eco = false;
          break;
        }
        if(read_digi(button_select)){          
          mode = 0;
          write_backlight_eco(true);
          lcd_backlight_eco = true;
          break;
        }
        if(read_digi(button_return)){
          mode = 0;
          break;
        }
        //切り替え実装
        print_flag = !print_flag;
        if(print_flag){
          lcd.setCursor(0, 1);
          lcd_print(alwayson);
        }
        else{
          lcd.setCursor(0, 1);
          lcd_print(ecomode);
        }
        delay(400);
      }
      break; 
      
    case 4:
      power_setting();
      standby(button_select);
      while(true){
        power_setting();
        if(read_digi(button_select)){
          mode = 0;
          break;
        }
        if(read_digi(button_enter)){          
          mode = 40;
          break;
        }
        if(read_digi(button_return)){
          mode = 0;
          break;
        }
        delay(100);
      }
      break;
      
    case 40:
      print_flag = false;
      lcd.clear();
      lcd_print(powersend);
      lcd.setCursor(0, 1);
      lcd_print(powersendstring);
      standby(button_enter);
      while(true){
        lcd.clear();
        lcd_print(powersend);
        if(read_digi(button_enter)){      
          mode = 0;
          write_power_send(true);
          power_send = true;
          break;
        }
        if(read_digi(button_select)){          
          mode = 0;
          write_power_send(false);
          power_send = false;
          break;
        }
        if(read_digi(button_return)){
          mode = 0;
          break;
        }
        //切り替え実装
        print_flag = !print_flag;
        if(print_flag){
          lcd.setCursor(0, 1);
          lcd_print(powersendstring);
        }
        else{
          lcd.setCursor(0, 1);
          lcd_print(powernotsendstring);
        }
        delay(400);
      }
      break; 
    
    default:
      mode = 0;
      break;
  }
  
}


bool read_digi(int port_num){//ボタン検出用の関数
  int on=0;
  for(int i=0;i<=10;i++){
    if(digitalRead(port_num))
      on++;
    delay(3);
  }
  if(on > 5){
    lcd_offtime = lcd_ontime + millis();
    return true;
  }
  return false;
}

void standby(int port_num){//ボタン入力後の待機関数
  while(read_digi(port_num)){
    delay(100);
  }
}

void standby2(int port_num,int port_num2){//ボタン入力後の待機関数(2ボタン)
  while(read_digi(port_num)||read_digi(port_num2)){
    delay(98);
  }
}

void standby3(int port_num,int port_num2,int port_num3){//ボタン入力後の待機関数(3ボタン)
  while(read_digi(port_num)||read_digi(port_num2)||read_digi(port_num3)){
    delay(98);
  }
}

void succsessfully(){//成功告知画面を表示する関数
  lcd.clear();
  lcd.print("succsessfully!");
  lcd.setCursor(0, 1);
  for(int i=0;i<=15;i++){
    lcd.print("*");
    delay(100);
  }
}

bool status_lcd(){
  lcd_clear();
  if(now_voltage < 10.0){
    lcd_print(warning);
    lcd.setCursor(0, 1);
    lcd_print(breakertripped);
    return false;
  }
  else if(now_voltage < 80.0){
    lcd_print(warning);
    lcd.setCursor(0, 1);
    lcd_print(lowvoltage);
    return false;
  }
  else{
    lcd_print(fine);
    lcd.setCursor(0, 1);
    lcd.print("No Problem");
    return true;
  }
}

void lcd_power(){
  lcd_clear();
  lcd_print(powerstring);
  lcd.setCursor(0, 1);
  lcd.print(now_power);
  lcd.setCursor(7, 1);
  lcd.print("[W]");
  if(stop_xbee)
    lcd.print(" Lock");
  delay(200);
}

void lcd_factor(){
  lcd.clear();
  lcd_print(factorstring);
  lcd.setCursor(0, 1);
  lcd.print(powerFactor*100.0);
  lcd.setCursor(7, 1);
  lcd.print("[%]");
}

void lcd_va(){
  lcd.clear();
  lcd_print(vastring);
  lcd.setCursor(0, 1);
  lcd.print(va);
  lcd.setCursor(7, 1);
  lcd.print("[VA]");
}

void lcd_current(){
  lcd.clear();
  lcd_print(currentstring);
  lcd.setCursor(0, 1);
  lcd.print(now_current);
  lcd.setCursor(7, 1);
  lcd.print("[A]");
}

void lcd_voltage(){
  lcd.clear();
  lcd_print(voltagestring);
  lcd.setCursor(0, 1);
  lcd.print(now_voltage);
  lcd.setCursor(7, 1);
  lcd.print("[V]");
}

void lcd_test1(){
  lcd.clear();
  lcd_print(test1);
  lcd.setCursor(0, 1);
  lcd_print(enter);
}

void lcd_test2(){
  lcd.clear();
  lcd_print(test2);
  lcd.setCursor(0, 1);
  lcd_print(enter);  
}

void lcd_test3(){
  lcd.clear();
  lcd_print(test3);
  lcd.setCursor(0, 1);
  lcd_print(enter);  
}

void power_setting(){
  lcd.clear();
  lcd_print(powersetting);
  lcd.setCursor(0, 1);
  lcd_print(enter);
}

