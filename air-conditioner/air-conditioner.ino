#include <DHT.h>
#include <DHT_U.h>

#include <pm2008_i2c.h>
#include <U8glib.h>
#include <MQUnifiedsensor.h>

#define Board   ("Arduino UNO")
#define ADC_Bit_Resolution      (10)
#define MQ_Voltage_Resolution   (5)

#define MQ2_Pin                 (A0)
#define MQ3_Pin                 (A1)
#define MQ9_Pin                 (A2)
#define Brightness_Pin          (A3)
#define MQ2_Digital_Pin         (2)
#define MQ3_Digital_Pin         (3)
#define DHT_Pin                 (4)
#define Buzzer_Pin              (5)
#define LED_R_Pin               (9)
#define LED_G_Pin               (10)
#define LED_B_Pin               (11)

#define DHT_Type                (DHT22)
#define RatioMQ2CleanAir        (9.83) //RS / R0 = 9.83 ppm 
#define RatioMQ3CleanAir        (60) //RS / R0 = 60 ppm 
#define RatioMQ9CleanAir        (9.6) //RS / R0 = 60 ppm 

#define LineHeight              (8)
#define Line1                   (0)
#define Line2                   (LineHeight*1)
#define Line3                   (LineHeight*2)
#define Line4                   (LineHeight*3)
#define Line5                   (LineHeight*4)
#define Line6                   (LineHeight*5)
#define Line7                   (LineHeight*6)
#define Line8                   (LineHeight*7)

PM2008_I2C pm2008_i2c;
DHT dht(DHT_Pin, DHT_Type);

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_DEV_0|U8G_I2C_OPT_NO_ACK|U8G_I2C_OPT_FAST);  // Fast I2C / TWI 

MQUnifiedsensor MQ2(Board, MQ_Voltage_Resolution, ADC_Bit_Resolution, MQ2_Pin, "MQ2");
MQUnifiedsensor MQ3(Board, MQ_Voltage_Resolution, ADC_Bit_Resolution, MQ3_Pin, "MQ3");
MQUnifiedsensor MQ9(Board, MQ_Voltage_Resolution, ADC_Bit_Resolution, MQ9_Pin, "MQ9");

MQUnifiedsensor* MQs[3] = {&MQ2, &MQ3, &MQ9};
float RatioMQs[3] = {RatioMQ2CleanAir, RatioMQ3CleanAir, RatioMQ9CleanAir};
float LPG, Methane, CO, Alcohol, H2, Propane, Benzene, Hexane;
float humidity, temperature;
char LPG_str[20], Alcohol_str[20], CO_str[20], temp_str[20], humid_str[20];
char buf[128];
long long now_time, pre_time1, pre_time2;
int brightness;

bool MQ2_digital, MQ3_digital;
uint8_t ret = 0, pm1p0_grade = 0, pm2p5_grade = 0, pm10p_grade = 0, total_grade = 0;
void setup() {
  pinMode(MQ2_Digital_Pin, INPUT);
  pinMode(MQ3_Digital_Pin, INPUT);
  pinMode(Brightness_Pin, INPUT);
  pinMode(Buzzer_Pin, OUTPUT);
  pinMode(LED_R_Pin, OUTPUT);
  pinMode(LED_G_Pin, OUTPUT);
  pinMode(LED_B_Pin, OUTPUT);
  
  Serial.begin(115200);
  dht.begin();
  MQ_Calibration();
  u8g.setFont(u8g_font_04b_03b);
  u8g.setFontPosTop();

  pm2008_i2c.begin();
  pm2008_i2c.command();
  delay(1000);
}

void loop() {
  u8g.firstPage();
  do{
    now_time = millis();
    if (now_time-pre_time1 > 500){
      Read_MQ2(); Read_MQ3();  Read_MQ9();
      pre_time1 = now_time;
    }
    if (now_time-pre_time2 > 3000){
      check_air();
      humidity = dht.readHumidity();
      temperature = dht.readTemperature();
      pre_time2 = now_time;
    }
    if(MQ2_digital || MQ3_digital){ tone(5, 670); delay(75); noTone(5); }
    else noTone(5);
    LED_Control();
    draw();
  }while(u8g.nextPage());
}

void MQ_Calibration(){
  for(int i = 0; i < 3; i++){
    MQs[i]->setRegressionMethod(1); //_PPM =  a*ratio^b
    MQs[i]->init();
    Serial.print("Calibrating please wait.");
    float calcR0 = 0;
    for(int j = 1; j<=10; j++)
    {
      MQs[i]->update();
      calcR0 += MQs[i]->calibrate(RatioMQs[i]);
      Serial.print(".");
    }
    MQs[i]->setR0(calcR0/10);
    Serial.println("  done!.");
    
    if(isinf(calcR0)) {Serial.println("Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply"); while(1);}
    if(calcR0 == 0){Serial.println("Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply"); while(1);}
  }
}

void Read_MQ2(){
  MQ2_digital = !digitalRead(2);
  MQ2.update();
  //MQ2.setA(987.99); MQ2.setB(-2.162); H2 = MQ2.readSensor();
  //MQ2.setA(574.25); MQ2.setB(-2.222); LPG = MQ2.readSensor();
  //MQ2.setA(36974); MQ2.setB(-3.109); CO = MQ2.readSensor();
  MQ2.setA(3616.1); MQ2.setB(-2.675); Alcohol = MQ2.readSensor();
  //MQ2.setA(658.71); MQ2.setB(-2.168); Propane = MQ2.readSensor();
}
void Read_MQ3(){
  MQ3_digital = !digitalRead(3);
  //MQ3.update();
  //MQ3.setA(44771); MQ3.setB(-3.245); LPG = MQ3.readSensor();
  //MQ3.setA(521853); MQ3.setB(-3.821); CO = MQ3.readSensor();
  //MQ3.setA(0.3934); MQ3.setB(-1.504); Alcohol = MQ3.readSensor();
  //MQ3.setA(4.8387); MQ3.setB(-2.68); Benzene = MQ3.readSensor();
  //MQ3.setA(7585.3); MQ3.setB(-2.849); Hexane = MQ3.readSensor();
}
void Read_MQ9(){
  MQ9.update();
  MQ9.setA(1000.5); MQ9.setB(-2.186); LPG = MQ9.readSensor();
  //MQ9.setA(4269.6); MQ9.setB(-2.648); Methane = MQ9.readSensor();
  MQ9.setA(599.65); MQ9.setB(-2.244); CO = MQ9.readSensor();
}

void draw(){
  dtostrf(Alcohol, 1, 1, Alcohol_str);
  dtostrf(LPG, 1, 1, LPG_str);
  dtostrf(CO, 1, 1, CO_str);
  dtostrf(temperature, 1, 1, temp_str);
  dtostrf(humidity, 1, 1, humid_str);
  
  u8g.drawStr( 0,Line1, "ALCOHOL");
  u8g.drawStr(40,Line1, (String(Alcohol_str)+" ppm").c_str());
  
  u8g.drawStr( 0,Line2, "LPG");
  u8g.drawStr(40,Line2, (String(LPG_str)+" ppm").c_str());
  
  u8g.drawStr( 0,Line3, "CO");
  u8g.drawStr(40,Line3, (String(CO_str)+" ppm").c_str());

  u8g.drawStr( 0,Line4, "pm1.0");
  u8g.drawStr(30,Line4, (String(pm2008_i2c.pm1p0_grimm)+" ppm").c_str());

  u8g.drawStr( 0,Line5, "pm2.5");
  u8g.drawStr(30,Line5, (String(pm2008_i2c.pm2p5_grimm)+" ppm").c_str());

  u8g.drawStr( 0,Line6, "pm10");
  u8g.drawStr(30,Line6, (String(pm2008_i2c.pm10_grimm)+" ppm").c_str());

  u8g.drawStr(75,(Line4+Line5)/2, "Temp");
  u8g.drawStr(100,(Line4+Line5)/2, (String(temp_str)+" 'C").c_str());
  
  u8g.drawStr(75,(Line5+Line6)/2, "Humid");
  u8g.drawStr(100,(Line5+Line6)/2, (String(humid_str)+" %").c_str());

  u8g.drawLine(0,Line4-1,127,Line4-1);
  u8g.drawLine(0,Line7-1,127,Line7-1);
  u8g.drawLine(70,Line4-1,70,63);
}

void check_air(){
  ret = pm2008_i2c.read();

  if (ret == 0) {
    // PM 1.0
    if (pm2008_i2c.pm1p0_grimm < 16) {
      pm1p0_grade = 1;
    } else if (pm2008_i2c.pm1p0_grimm < 51) {
      pm1p0_grade = 2;
    } else if (pm2008_i2c.pm1p0_grimm < 101) {
      pm1p0_grade = 3;
    } else {
      pm1p0_grade = 4;
    }

    // PM 2.5
    if (pm2008_i2c.pm2p5_grimm < 16) {
      pm2p5_grade = 1;
    } else if (pm2008_i2c.pm2p5_grimm < 51) {
      pm2p5_grade = 2;
    } else if (pm2008_i2c.pm2p5_grimm < 101) {
      pm2p5_grade = 3;
    } else {
      pm2p5_grade = 4;
    }

    // PM 10
    if (pm2008_i2c.pm10_grimm < 31) {
      pm10p_grade = 1;
    } else if (pm2008_i2c.pm10_grimm < 81) {
      pm10p_grade = 2;
    } else if (pm2008_i2c.pm10_grimm < 151) {
      pm10p_grade = 3;
    } else {
      pm10p_grade = 4;
    }

    // Get worst grade
    total_grade = max(pm1p0_grade, pm2p5_grade);
    total_grade = max(total_grade, pm10p_grade);
  }
}

void LED_Control(){
  brightness = analogRead(Brightness_Pin);
  brightness = map(brightness,0,1024,0,255);
  switch (total_grade) {
    case 1: {
        //Good
        analogWrite(LED_R_Pin, brightness);
        analogWrite(LED_G_Pin, brightness);
        analogWrite(LED_B_Pin, brightness);
        break;
      }
    case 2: {
        // Normal
        analogWrite(LED_R_Pin, 0);
        analogWrite(LED_G_Pin, 0);
        analogWrite(LED_B_Pin, brightness);
        break;
      }
    case 3: {
        // Bad
        analogWrite(LED_R_Pin, 0);
        analogWrite(LED_G_Pin, brightness);
        analogWrite(LED_B_Pin, 0);
        break;
      }
    case 4: {
        // Worst
        analogWrite(LED_R_Pin, brightness);
        analogWrite(LED_G_Pin, 0);
        analogWrite(LED_B_Pin, 0);
        break;
      }
    default:
      break;
  }
}
