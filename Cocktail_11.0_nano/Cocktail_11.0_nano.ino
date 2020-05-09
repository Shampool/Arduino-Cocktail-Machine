/*
Version: Cocktail 11.0 (nano clean version);
Author: Shampool
Date: 2020/5/10
Steps: 0-welcome; 1-encoder; 2-HX711+motor
Old functions: 
        pressing an encoder to go to step 1; rotating the encoder to indicate the volume of the drink, pressing it to confirm and going to step 2
        Limiting the range of liquid volume;
        Zeroing the value of variable 'counter' in Step-0
        Adding a loop for LCDencoder to input the volume of the three drinks
        Elimiating the arguments 'Step'
        HX711 can weight three times in an entire loop;
        and each time, the value of weight will be set to zero before weighting;
        HX711 will stop weighting when the value of weight is larger than the Drink[index];       
        driving the three motors;
        All the expected functions have been completed!
        all the Serial port statements were deleted;
        redundant annotated functions were deleted;
        combined the self-defined function 'HX711_weight()' to the default function 'loop';
        Using Nano to take place of UNO
        User need to finally confirm by pressing once, otherwise the operation will be cancelled after 10 seconds;
Problem:
        'ifpressed' in row 118 will change the value of Drink[3], so we need a variable 'Drink3' to store the value of Drink[3] temporarily. The testing-used sentences were retained
        this problem has been solved by some tricks, but the real reason hasn't been found
New function:
        nothing new but deleting some unnecessary sentences, including the testing sentences used in Version 10.0_nano;
        some serial sentences were retained to make future optimization convenient;
*/

#include<HX711_ADC.h>
#include<LiquidCrystal_I2C.h>
#include<Wire.h>
#define DT 4    // rotary encoder D4~
#define CLK 3   // rotary encoder D3
#define SW 2    // rotary encoder D2
#define PUMP_1 10    // L2987N_1 1st_motor, pin IN2 of the 1st L298N, D9
#define PUMP_2 9   // L2987N_1 2nd_motor, pin IN4 of the 1st L298N, D10
#define PUMP_3 7   // L2987N_2 3rd_motor, pin IN4 of the 2nd L298N, D7
#define HX_CLK 6  // CLK of HX711 D8
#define HX_DT 5 // Digital output of HX711 D5~

int Step = 0;       // 步骤，按一下+1，连按两下-1
int counter = 0;          // 设定的毫升数，用encoder计数
int currentStateCLK;      // for determining the amount of rotation
int lastStateCLK;         // for determining the amount of rotation
String currentDir = "";   // will be used when printing the current direction of rotation on the serial monitor
unsigned long lastButtonPress = 0;    // debounce a switch
int Drink[3];       // used for store the volume of each kind of drinks
float weight;       // HX711 weighting
int ifpressed = 0;  // 按键初始状态-未被按下
int index;          // the label of drinks
bool cancel = 0;     // cancel = 1 means cancel the production

HX711_ADC LoadCell(HX_DT, HX_CLK); //define HX711 as 'LoadCell', DT pin-D9~, SCK pin-D8
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // 設定 LCD I2C 位址

void setup() {
  Serial.begin(9600);   // 开启序列埠
  Serial.println("Welcome to Cocktail Party");
  Serial.println("Readings:");
/*== LCD ==*/
  lcd.begin(16, 2);   // 打开LCD
  lcd.backlight();    // 开启背光
  lcd.setCursor(0, 0);
  lcd.print("Welcome!        ");
  lcd.setCursor(0, 1);
  lcd.print("Cocktail Machine");
/*== rotary encoder ==*/
  pinMode(CLK, INPUT);              // the primary output pulse for determing the amout of rotation
  pinMode(DT, INPUT);               // is to determine the direction of rotation
  pinMode(SW, INPUT_PULLUP);        // Pull-up resistor
  lastStateCLK = digitalRead(CLK);  // reads the initial state of CLK from a digital pin, either HIGH or LOW
/*== HX711 ==*/
  LoadCell.begin();                 // connect HX711  打开HX711
  LoadCell.start(2000);             // 2000ms fot stabilization
  LoadCell.setCalFactor(400.0);     // calibration factor
  lcd.clear();                      // LCD清屏

  pinMode(PUMP_1, OUTPUT);             // PUMP_1 outputs signals to drive the 1st motor
  pinMode(PUMP_2, OUTPUT);             // PUMP_2 outputs signals to drive the 2nd motor
  pinMode(PUMP_3, OUTPUT);             // PUMP_3 outputs signals to drive the 3rd motor
}

void loop() {
  /*=====Step 0 ===== Welcome =====*/
  while (1) {              // LCD: welcome
    lcd.setCursor(0, 0);
    lcd.print("Welcome!        ");
    lcd.setCursor(0, 1);
    lcd.print("Press to start");
    counter = 0;          // zeroing the value of counter
    
    ButtonState();        // self-defined function, 检测按键是否被按下
    if (ifpressed == 1) { // if pressed
      ifpressed = 0;      // 按键参数置零
      break;              // displays 'welcome' until the button was pressed
    }
  }
  
/*===== Step 1 ===== Setting Drink ABC =====*/
  for (int index=1; index<4; index++){
    counter = 0;          // zeroing the volume
    LCDencoder(index);    // Input the volume of the drinks in turns
    Drink[index] = counter; // save the value of volume of each drink to the array, which will be used in 'motor' section
  }
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Press to confirm"); // press to make, or wait for 5s and cancel production
  lcd.setCursor(0, 1);
  lcd.print(Drink[1]);
  lcd.print(" ");
  lcd.print(Drink[2]);
  lcd.print(" ");
  lcd.print(Drink[3]);
  delay(1000);

int Drink3 = Drink[3];    // 臨時定義一個變量存儲Drink[3]的值，否則該值將被賦予與125行前的ifpressed相同的值

  while(1)  {             // final confirm
    ButtonState();        // detect whether the button was pressed
    if (ifpressed == 1) { //按键参数为1，被按下，打印出来
      ifpressed = 0;      // 按键参数置零  ！！！問題：此處的操作會使Drink[3]=ifpressed
      cancel = 0;         // confirm to make      
      Drink[3]=Drink3;    // reassign Drink[3]
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Making ");
      lcd.setCursor(0, 1);
      lcd.print("Please wait... ");
      break;              // jump out from while loop  
    }
    else if (millis() - lastButtonPress > 5000 ) {    // over 5s
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cancelled");
      cancel = 1;         // cancel the production
      delay(1000);
      break;
    }
  }

/*===== Step 2 == HX711 measures the weight + motor=====*/
  if (cancel == 0){
    for (index=1; index<4; index++){
      delay(2000);
      LoadCell.tare();  // reset HX711 to 0 to weight the next drink
      weight=0;         // zero the weight
      lcd.clear();            // LCD清屏
      lcd.setCursor(0, 0);
      lcd.print("Drink ");
      lcd.print(index);
      lcd.print(" [ml]:");
      lcd.print(Drink[index]);
      while(weight<Drink[index]){ // drives the corresponding motor until reaching the pre-setted value
        LoadCell.update();        // retrieves data from the load cell
        weight = LoadCell.getData(); // get value of weight
        Serial.print("weight is ");
        Serial.print(weight, 1);  //scale.get_units() returns a float
        Serial.println(" g");     //You can change this to kg but you'll need to refactor the calibration_factor  
        delay(100);
        lcd.setCursor(0, 1);    //设定光标至第二行
        lcd.print(weight);      //在LCD上打印重量
        lcd.print("   ");
        
        switch(index) {             // drive the pumps
          case 1: Serial.println("PUMP1"); digitalWrite(PUMP_3, LOW); digitalWrite(PUMP_1, HIGH); break;
          case 2: Serial.println("PUMP2"); digitalWrite(PUMP_1, LOW); digitalWrite(PUMP_2, HIGH); break;
          case 3: Serial.println("PUMP3"); digitalWrite(PUMP_2, LOW); digitalWrite(PUMP_3, HIGH); break;
          }
      }
      digitalWrite(PUMP_1, LOW); digitalWrite(PUMP_2, LOW); digitalWrite(PUMP_3, LOW); 
      delay(1000);
      lcd.setCursor(0, 1);
      lcd.print("Done!        ");
    }
    delay(2000);
    lcd.clear();            // LCD清屏
    lcd.setCursor(0, 0);
    lcd.print("Completed!     ");
    delay(3000);
  }
}

void LCDencoder(int index) {       // Input the volume of the drinks in turns
  lcd.clear();            // LCD清屏
  lcd.setCursor(0, 0);
  lcd.print("Drink ");
  lcd.print(index);
  lcd.print(" [ml]:");    //第一行字可以放到小循环外

  while (1){
    lcd.setCursor(0, 1);
    lcd.print(counter);   //在LCD第二行打出设定的毫升数
    lcd.print("  ");      //清除LCD右位残影
    currentStateCLK = digitalRead(CLK);  // Read the current state of CLK

    if (currentStateCLK != lastStateCLK  && currentStateCLK == 1) { //检测是否有变化
      
      if (digitalRead(DT) != currentStateCLK) {   //检测转动方向
        if(counter>=50){counter--;}   // upper limit
        counter ++;
        currentDir = "CCW";
      } 
      else {
        if(counter<=0){counter++;}    // lower limit
        counter --;
        currentDir = "CW";
      }
      Serial.print("Direction: ");
      Serial.print(currentDir);
      Serial.print(" | Counter: ");
      Serial.println(counter);
    }
    
    lastStateCLK = currentStateCLK;  // Remember last CLK state

    ButtonState();  //检测按键是否被按下
    if (ifpressed == 1) { //按键参数为1，被按下，打印出来
      lcd.setCursor(0, 1);
      lcd.print("Confirmed! ");
      lcd.print(counter);
      delay(1000);
      ifpressed = 0;      // 按键参数置零
      break;    // jump out from while loop
    }
    delay(1);
  }
}
 
void ButtonState() {      // Determine whether the button is pressed
  int btnState = digitalRead(SW);  // Read the button state
  
  if (btnState == LOW) {    // Button and Debounce 按键及按键消抖
    if (millis() - lastButtonPress > 50) {  // 距上次LOW的时长大于50ms，否则认为是键抖产生的错误按下
      Serial.println("Button pressed!");
      ifpressed = 1; // 按键参数为1，说明被按下
    }
    lastButtonPress = millis();    // Remember last button press event
  }
}
