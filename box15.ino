#include<XBeeLibrary.h>
#include<SoftwareSerial.h>
#include<LiquidCrystal.h>
#include<EEPROM.h>
//#include<avr/wdt.h>
#include <Scheduler.h>
#include <TimerOne.h>
#include <MsTimer2.h>
XBeeLibrary XBee;

/*電力計測関係*/
#define SAMPLE_PER_SEC 600
#define SAMPLE_PERIOD_USEC (1000000 / SAMPLE_PER_SEC)
#define NOISE_WATT_THRESHOLD 0.5           // これ未満のW数ならノイズとして捨てる

/*入出力関係*/
#define button_select A0//選択ボタン接続端子
#define button_enter A1//決定ボタン接続端子
#define button_return A2//戻るボタン接続端子
#define current_vol  A3//電流センサ接続端子
#define voltage_vol A4//電圧測定端子
#define refarence_vol A5//校正用電圧入力端子 2.5Vを入力

#define lcd_backlight 4//LCDのバックライト制御ラインを接続する端子
#define lcd_ontime 300000//LCDのバックライトの点灯時間[ms]
#define backlight_setting_address 0

SoftwareSerial SSerial(6,5);//ソフトウェアシリアル

LiquidCrystal lcd(7,8,9,10,11,12);//LCD

volatile double now_power = 0.0;
volatile double now_voltage = 0.0;
volatile double now_current = 0.0;
volatile double powerFactor = 0.0;
volatile double va = 0.0;
volatile unsigned long total_power = 0.0;

bool stop_xbee = false;
bool wait_lcd = false;
bool lcd_backlight_eco;
unsigned long lcd_offtime = lcd_ontime + 3000;//30000;

char enter[] = "Enter:ｹｯﾃｲ";
char testmode[] = "ﾃｽﾄﾓｰﾄﾞ";
char selectmanagement[] = "Select:ｼｽﾃﾑｶﾝﾘ";
char test1[] = ">Test 1(ｾﾞﾝﾃﾝﾄｳ)";
char test2[] = ">Test 2(ﾊﾟﾀｰﾝ)";
char test3[] = ">Test 3(ｼｮｳﾄｳ)";
char management[] = "ﾃﾞﾝﾘｮｸｶﾝﾘ";
char setting[] = "ｾｯﾃｲ";
char selectsetting[] ="Select:ｾｯﾃｲ";
char powerstring[] = "ｼｮｳﾋﾃﾞﾝﾘｮｸ";
char factorstring[] = ">ﾘｷﾘﾂ";
char vastring[] = ">ﾋｿｳﾃﾞﾝﾘｮｸ";
char alwayson[] ="Enter:ﾂﾈﾆﾃﾝﾄｳ";
char ecomode[] ="Select:ｴｺﾓｰﾄﾞ";
char lcdbacklight[] = "LCDﾊﾞｯｸﾗｲﾄ";
char warning[] = "ｼﾞｮｳﾀｲ:ｹｲｺｸ!";
char lowvoltage[] = "ﾃﾞﾝｱﾂｶﾞﾃｲｶｼﾃｲﾏｽ";
char breakertripped[] = "ﾌﾞﾚｰｶｶﾞｵﾁﾃｲﾏｽ";
char fine[] = "ｼｽﾃﾑﾊｾｲｼﾞｮｳﾃﾞｽ";
char exitreturn[] = "ﾓﾄﾞﾙ:return";
char currentstring[] = ">ﾃﾞﾝﾘｭｳ";
char voltagestring[] = ">ﾃﾞﾝｱﾂ";

int mode = 100;
/*
  0 is Power mode 
  -> 100 is Power mode(Non Standby)  
  1 is Test　Signal Select mode
  -> 10 all on 
  -> 11 blinks
  2 is configure mode 
*/

int pattern_mode = 0;//2017/7/25 スケジューラの動作不調により廃止
/*
  0 is Do nothing
  1 is lcd blinks
  2 is xbee blinks
  other(default) already used
*/

void char_setup(){
  utf_del_uni(enter);
  utf_del_uni(testmode);
  utf_del_uni(selectmanagement);
  utf_del_uni(test1);
  utf_del_uni(test2);
  utf_del_uni(test3);
  utf_del_uni(management);
  utf_del_uni(setting);
  utf_del_uni(selectsetting);
  utf_del_uni(powerstring);
  utf_del_uni(factorstring);
  utf_del_uni(vastring);
  utf_del_uni(alwayson);
  utf_del_uni(ecomode);
  utf_del_uni(lcdbacklight);
  utf_del_uni(warning);
  utf_del_uni(breakertripped);
  utf_del_uni(lowvoltage);
  utf_del_uni(fine);
  utf_del_uni(exitreturn);
  utf_del_uni(currentstring);
  utf_del_uni(voltagestring);
}

void setup(){
  XBee.setup(true);
  XBee.setAddress(0x0013A200, 0x40B401BD);//1 Coordinator_address
  Serial.begin(9600);
  SSerial.begin(9600);
  
  pinMode(13,OUTPUT);
  digitalWrite(13,LOW);
  
  pinMode(lcd_backlight,OUTPUT);
  digitalWrite(lcd_backlight,HIGH);

  char_setup();
  lcd.begin(16, 2);

  ADCSRA = ADCSRA & 0xf8;//A/Dコンバータの高速化
  ADCSRA = ADCSRA | 0x04;
  resetSecValues();

  //Scheduler.startLoop(pattern_loop);
  Scheduler.startLoop(xbee_loop);
  
  Timer1.initialize(SAMPLE_PERIOD_USEC);
  Timer1.attachInterrupt(sample);
  
  lcd_backlight_eco = read_backlight_eco();
  
  char text[] = "ﾏｲﾂﾞﾙｺｳｾﾝ";
  utf_del_uni(text);
  lcd_print_string(text);

  lcd.setCursor(0, 1);
  char text1[] = "4E ｿｳｿﾞｳｺｳｶﾞｸ";
  utf_del_uni(text1);
  lcd_print_string(text1);
  
  delay(2000);
  lcd.clear();
  lcd_print_string("Smart Power");

  lcd.setCursor(1, 1);
  lcd_print_string("Control Unit");

  delay(2000);
  
  status_lcd();
  delay(2000);
  
  //wdt_enable(WDTO_4S);//ウォッチドッグタイマーを有効化
  
}

void loop(){//LCDの表示を制御
  unsigned int count;
  bool print_flag;
  switch(mode){
    case 0:
      lcd_power();
      standby2(button_select,button_enter);
    case 100:
      while(true){
        if(now_voltage <= 80.0)
          status_lcd();
        else
          lcd_power();
        if(read_digi(button_select)||read_digi(button_enter)){
          mode = 1;
          break;
        }
        if((lcd_offtime <= millis())&&lcd_backlight_eco)
          back_light_off();
      }
      
      break;
      
    case 1:
      count = 1;
      print_flag = false;
      lcd.clear();
      lcd_print_string(testmode);
      lcd.setCursor(0, 1);
      lcd_print_string(enter);
      standby2(button_select,button_enter);
      while(true){
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
        if(count % 12 == 0){
          print_flag = !print_flag;
          if(print_flag){
            lcd.setCursor(0, 1);
            lcd_print_string(enter);
            lcd_print_string("    ");
          }
          else{
            lcd.setCursor(0, 1);
            lcd_print_string(selectmanagement);
          }
        }
        delay(100);
        count++;
      }
      break;

    case 10:
      lcd.clear();
      lcd_print_string(test1);
      lcd.setCursor(0, 1);
      lcd_print_string(enter);
      standby(button_enter);
      while(true){
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
      }
      break;

    case 11:
      lcd.clear();
      lcd_print_string(test2);
      lcd.setCursor(0, 1);
      lcd_print_string(enter);
      standby(button_select);
      while(true){
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
      }
      break;
      
    case 12:
      lcd.clear();
      lcd_print_string(test3);
      lcd.setCursor(0, 1);
      lcd_print_string(enter);
      standby(button_select);
      while(true){
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
      }
      break;
      
    case 2:
      count = 1;
      print_flag = false;
      lcd.clear();
      lcd_print_string(management);
      lcd.setCursor(0, 1);
      lcd_print_string(enter);
      standby(button_select);
      while(true){
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
        if(count % 12 == 0){
          print_flag = !print_flag;
          if(print_flag){
            lcd.setCursor(0, 1);
            lcd_print_string(enter);
          }
          else{
            lcd.setCursor(0, 1);
            lcd_print_string(selectsetting);
          }
        }
        delay(100);
        count++;
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
      lcd.clear();
      lcd_print_string(setting);
      lcd.setCursor(0, 1);
      lcd_print_string(enter);
      standby(button_select);
      while(true){
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
      }
      break;

    case 30:
      count = 1;
      print_flag = false;
      lcd.clear();
      lcd_print_string(lcdbacklight);
      lcd.setCursor(0, 1);
      lcd_print_string(alwayson);
      standby(button_enter);
      while(true){
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
        if(count % 12 == 0){
          print_flag = !print_flag;
          if(print_flag){
            lcd.setCursor(0, 1);
            lcd.print(alwayson);
          }
          else
          {
            lcd.setCursor(0, 1);
            lcd.print(ecomode);
          }
        }
        delay(100);
        count++;
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
    delay(5);
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
  lcd_print_string("succsessfully!");
  lcd.setCursor(0, 1);
  for(int i=0;i<=15;i++){
    lcd.print("*");
    delay(100);
  }
}

void short_succsessfully(){//成功告知画面を表示する関数
  lcd.clear();
  lcd_print_string("succsessfully!");
  lcd.setCursor(0, 1);
  for(int i=0;i<=15;i++){
    lcd.print("*");
    delay(50);
  }
}

bool status_lcd(){
  lcd.clear();
  if(now_voltage < 10.0){
    lcd_print_string(warning);
    lcd.setCursor(0, 1);
    lcd_print_string(breakertripped);
    return false;
  }
  else if(now_voltage < 80.0){
    lcd_print_string(warning);
    lcd.setCursor(0, 1);
    lcd_print_string(lowvoltage);
    return false;
  }
  else{
    lcd_print_string(fine);
    lcd.setCursor(0, 1);
    lcd_print_string("No Problem");
    return true;
  }
}

void lcd_power(){
  lcd.clear();
  lcd_print_string(powerstring);
  lcd.setCursor(0, 1);
  lcd_print_double(now_power);
  lcd.setCursor(7, 1);
  lcd_print_string("[W]");
  delay(200);
}

void lcd_factor(){
  lcd.clear();
  lcd_print_string(factorstring);
  lcd.setCursor(0, 1);
  lcd_print_double(powerFactor*100.0);
  lcd.setCursor(7, 1);
  lcd_print_string("[%]");
}

void lcd_va(){
  lcd.clear();
  lcd_print_string(vastring);
  lcd.setCursor(0, 1);
  lcd_print_double(va);
  lcd.setCursor(7, 1);
  lcd_print_string("[VA]");
}

void lcd_current(){
  lcd.clear();
  lcd_print_string(currentstring);
  lcd.setCursor(0, 1);
  lcd_print_double(now_current);
  lcd.setCursor(7, 1);
  lcd_print_string("[A]");
}

void lcd_voltage(){
  lcd.clear();
  lcd_print_string(voltagestring);
  lcd.setCursor(0, 1);
  lcd_print_double(now_voltage);
  lcd.setCursor(7, 1);
  lcd_print_string("[V]");
}


