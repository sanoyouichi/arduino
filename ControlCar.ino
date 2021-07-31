/*
 * ライブラリをarduinoに反映する方法
 * 
 * step1: 各リンクからライブラリをインストールしたのちに、「スケッチ」->「ファイルを追加」->ダウンロードしたファイルを選択
 * step2: 「スケッチ」->「ライブラリをインクルード」->先ほどインストールしたライブラリ名が一番下に表示されているので、それを選択
 * 
 */

#include <TimerOne.h>          // 赤外線反射センサーを制御するためのライブラリ (https://files.seeedstudio.com/wiki/Grove-Infrared_Reflective_Sensor/res/TimerOne-ArduinoLib.zip)
#include <SparkFun_TB6612.h>   // motorを制御するためのライブラリ (https://github.com/sparkfun/SparkFun_TB6612FNG_Arduino_Library/archive/master.zip)
#include <rgb_lcd.h>           // LCDを制御するためのライブラリ   (https://github.com/Seeed-Studio/Grove_LCD_RGB_Backlight/archive/master.zip)

#define MAGNECTIC_SWITCH 7     // 磁気センサーのportを定義(今回はD7)

#define AIN1 12  // motor1の入力portを定義
#define AIN2 9   // motor1の入力portを定義
#define PWMA 5   // motor1の出力portを定義

#define BIN1 4   // motor2の入力portを定義
#define BIN2 3   // motor2の入力portを定義
#define PWMB 11  // motor2の出力portを定義

#define STBY 10  

#define right_motor 8   // 右のモータへの出力portを定義
#define left_motor 6    // 左のモータへの出力portを定義

const int offsetA = 1;  // motorの定義ファイルの種類を選択
const int offsetB = 1;  // motorの定義ファイルの種類を選択

Motor motor1 = Motor(AIN1, AIN2, PWMA, offsetA, STBY);  //motor1を定義
Motor motor2 = Motor(BIN1, BIN2, PWMB, offsetB, STBY);  //motor2を定義

rgb_lcd lcd;             // LCDを制御するためのクラスをlcdと命名(命名しないのであれば、lcdの箇所をrgb_lcdとすればよい)
const int colorR = 139;  // LCDの背景色をRGBで決定
const int colorG = 0;    // LCDを背景色をRGBで決定
const int colorB = 0;    // LCDを背景色をRGBで決定

int  type       = 0;      // モーターの動きをtypeで管理している。詳細はturnOnLCD参照 (1:左に旋回, 2:両輪後進, 3:両輪前進, 4:右に旋回)
int  count      = 0;      // 磁気を検知した回数
bool count_flag = false;  // 磁気を初めて検知したかどうか（磁気を検知し続けるためにloop処理を使っているので、ループ処理の1回目のみカウントアップするように）

void setup() 
{
    // MOTOR
    Timer1.initialize(100);                  // 赤外線反射センサーが検知する単位時間（ms）
    Timer1.attachInterrupt( controlMotor );  // attach the service routine here
    // LCD
    pinMode(MAGNECTIC_SWITCH, INPUT);        // 磁気センサーからの入力portを定義
    lcd.begin(16, 2);                        // LCDに表示される上限は32bitなので、表示される形を16列2行と定義
    lcd.setRGB(colorR, colorG, colorB);      // LCDの背景色を定義
    lcd.print("I'm Lamborghini");            // LCDに表示される文字（なくてもよい）
}

void loop()
{
    // 磁気検知すればturnOnLCDを、されなければturnOffLCDを実行
    if(isNearMagnet()) {
      turnOnLCD();
    } else {
      turnOffLCD(); 
    }
}

void controlMotor()
{
    // 赤外線反射センサーとモーターを利用
    // 進行方向に向かって左右を決めている
    // motor1:左 motor2:右
    // digitalRead(port)で、white = 0, black = 1としてコース上を走る車を制御
    // ただ挙動が以下のようにズレており、原因はわかっていない（配線？？）
    // left ->forward
    // right->back
    // back-> left
    // forwrd->right
    // またモーターの左右が配線上とコード上で逆になっているので旋回の向きは逆
    if(digitalRead(left_motor) == 1  && digitalRead(right_motor) == 1) {
      type = 1;
      left(motor1, motor2,500);         // 左に旋回
    } else if(digitalRead(left_motor) == 1  && digitalRead(right_motor) == 0) {
      type = 2;
      forward(motor1, motor2, 500);     // 両輪前進
    } else if(digitalRead(left_motor) == 0  && digitalRead(right_motor) == 1) {
      type = 3; 
      back(motor1, motor2, 500);        // 両輪後進
    } else if(digitalRead(left_motor) == 0  && digitalRead(right_motor) == 0) {
      type = 4;
      right(motor1, motor2, 500);       // 右に旋回
    };
}

bool isNearMagnet()
{
    // 磁気センサーを利用
    int sensorValue = digitalRead(MAGNECTIC_SWITCH);
    if(sensorValue == HIGH){
      return true;
    } else {
      return false;
    }
}

void turnOnLCD() 
{  
    // 磁気センサーを利用
    // loop処理での1回目のみカウントアップ
    // (磁気センサーはloop処理の中で検知し続けるので制御しなければ
    // 検知範囲内に磁石がある限りカウントし続け16桁以上の数値をプリントする）
    // モーターを回しながらLCDを制御しようとすると、電池供給不足を起こし回路全体が止まってしまうことがあった
    // 対応として、カウントアップがあった際にモーターを止めてLCDを書き換え、それまでのモーターの動きを引き継ぐ
    // 引き継ぐためにモーターの動きをtypeで管理している
    // モーターが止まり、また動き出すまでの時間は十分に短いので無視して良い
    if(count_flag == false) {
      brake(motor1, motor2);
      count = count + 1; 
      lcd.setCursor(0, 1);             // プリントする位置を0列1行目に設定(プログラミングでは0が最初なので１行目を指定すると見た目は２行目に表示される)
      lcd.print(count);
      
      if(type == 1 ){
        left(motor1, motor2,250);      // 左に旋回
      } else if(type == 2){
        forward(motor1, motor2, 250);  // 両輪前進
      } else if(type == 3){
        back(motor1, motor2, 250);     // 両輪後進
      } else if(type == 4){
        right(motor1, motor2, 250);    // 右に旋回
      }

      // 磁石の上を車体が通る前にセンサーが一回検知する。
      // その後車体が上向きになり、センサーが検知しなくなる。
      // その後車体が下向きになり、また検知される。
      // という流れで検知回数がダブるので一度カウントアップしたあとは2000ms待つように
      delay(2000);
    }
    
    // loop処理での2回目以降はカウントアップ済みとしてカウントアップしないように
    count_flag = true;
}

void turnOffLCD()
{
    // 磁気センサーが検知しなくなればfalseに変更
    count_flag = false;
}
