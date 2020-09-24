#include <Arduino.h>
#include <SD.h>

#define CS_PIN D8

double Kp = 1;
double Ki = 0;
double Kd = 0;

double sample_period =10;
unsigned long last_time;
double total_error = 0;
double last_error = 0;
bool error_type = true; //false is negative error true is positive error
double control_signal;
double actual_ph_value = 0;
double desired_ph_value = 5.0;
double error;


bool log_data = true;
bool read_log_file = false;
unsigned long test_duration = 6000;
unsigned long test_start_time = 0;


void pid_control() {
  unsigned long current_time = millis();
  unsigned long delta_time = current_time - last_time;

  if (delta_time >= sample_period) {
    error = desired_ph_value - actual_ph_value;
   // if((error) == 0.00) total_error = 0;
  //  else  total_error += error;
    
  if (total_error >= 255) total_error = 255;
  if (total_error <= 0) total_error = 0;

  //check if error is positive or negative;
  if (error < 0) error_type = false;
  else if (error > 0) error_type = true;
  //-------------------------------------
  double delta_error = error - last_error;
  last_error = error;
  
  control_signal = (Kp * error) + (Ki * total_error * sample_period) + ((Kd / sample_period) * delta_error);
  if (control_signal >= 255) control_signal = 255;
  if (control_signal <= -255) control_signal = -255;
  
  last_time = current_time;

  }
}

void setup() {
   Serial.begin(115200);

  if (!SD.begin(CS_PIN)) {
    Serial.println("Error Initializing SD card");
  }
  
  else{
    Serial.println("SD card Initialized successfully");
  }

  //Check for pre_existing log file and erase
  if (SD.exists("PH_LOG.csv")) {
      SD.remove("PH_LOG.csv");
  } 

}

void parse_payload(String payload){
  //error,control_signal,desired_value,actual_value,time
  int error_end_index = payload.indexOf(',',0);
  int control_signal_end_index = payload.indexOf(',',error_end_index+1);
  int desired_value_end_index = payload.indexOf(',',control_signal_end_index+1);
  int actual_value_end_index = payload.indexOf(',',desired_value_end_index+1);

  String retreived_error = payload.substring(0,error_end_index);
  String retreived_control_signal = payload.substring(error_end_index+1,control_signal_end_index);
  String retreived_desired_value = payload.substring(control_signal_end_index+1,desired_value_end_index);
  String retreived_actual_value = payload.substring(desired_value_end_index+1,actual_value_end_index);
  String retreived_time = payload.substring(-1,actual_value_end_index+1);

  Serial.print("Error: ");
  Serial.print(retreived_error);
  Serial.print(" Control: ");
  Serial.print(retreived_control_signal);
  Serial.print(" Desired: ");
  Serial.print(retreived_desired_value);
  Serial.print(" Actual: ");
  Serial.print(retreived_actual_value);
  Serial.print(" Time: ");
  Serial.println(retreived_time);
}

void loop() {
  if((millis() - test_start_time) < test_duration){
    pid_control();
    //If we are active to log data
    if(log_data){ 
      //Open the logging file
      File dataFile = SD.open("PH_LOG.csv", FILE_WRITE);
        if (dataFile) { //If the logging file is sucessfully opened
          dataFile.print(error);
          dataFile.print(",");
          dataFile.print(control_signal);
          dataFile.print(",");
          dataFile.print(desired_ph_value);
          dataFile.print(",");
          dataFile.print(actual_ph_value);
          dataFile.print(",");
          dataFile.println(millis()-test_start_time);
          //Close the logging file
          dataFile.close();
        }
        else{
          Serial.println("ERROR Opening logging file");
        }

        Serial.print(error);
        Serial.print(",");
        Serial.print(control_signal);
        Serial.print(",");
        Serial.print(desired_ph_value);
        Serial.print(",");
        Serial.println(actual_ph_value);      
    }

    if(error_type == true){ //positive error
      actual_ph_value+=0.1;
    }
    if(error_type == false){ //negative error
      actual_ph_value -= 0.1;
    }

  }

    if((millis() - test_start_time) > test_duration){
      File dataFile = SD.open("PH_LOG.csv", FILE_READ);
      if(dataFile){
        while (dataFile.available()) {
          String payload = dataFile.readStringUntil('\n');
           parse_payload(payload);
          delay(1000);
        }

        dataFile.close();
   }
    }
}