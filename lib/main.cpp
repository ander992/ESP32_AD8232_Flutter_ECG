#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <ResponsiveAnalogRead.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

//Network credentials
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
//Firebase credentials
#define FIREBASE_API_KEY ""
#define FIREBASE_HOST "" 

//Firebase variables
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

void initWiFi(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
}

void initFirebase(){
  config.api_key = FIREBASE_API_KEY;
  config.database_url = FIREBASE_HOST;
  if(Firebase.signUp(&config, &auth, "", "")){
    Serial.println("Sign Up to Firebase succesful");
  } else {
    Serial.print("Error:");
    Serial.println(config.signer.signupError.message.c_str());
  }
  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Connected to Firebase");
}

//Here all the pins are defined
const int inputPin = GPIO_NUM_34; 

/*Pan-Tompkins algorithm parameters*/
#define M       5 // Here you can change the size for the Highpass filter
#define N       30 // Here you can change the size for the Lowpass filter
#define winSize     250   // this value defines the windowsize which effects the sensitivity of the QRS-detection.
#define HP_CONSTANT   ((float) 1 / (float) M)
#define MAX_BPM     100 // Set an upper threshold of BPM for a better detection rate
#define RAND_RES 100000000

ResponsiveAnalogRead analog(inputPin, true);

/* Important for calculating the between QRS interval.*/
int cprTimeRead_1 = 0; 
int cprTimeRead_2 = 0;
float difference = 0;
float interval;
unsigned long currentMillis = millis();
unsigned long previousMillis = 0;
int next_ecg_pt;
int QRS = 0;
int tmp = 0;       

/*timing variables for sending data to Firebase*/
unsigned long sendDataPrevMillis = 0;
unsigned long currentMicros = 0;
unsigned long previousMicros = 0;

/*variables for writing to a csv file over putty*/
char dataStr[100] = "";
char buffer[7];

/* Portion pertaining to Pan-Tompkins QRS detection */
// circular buffer for input ecg signal
// we need to keep a history of M + 1 samples for HP filter
float ecg_buff[M + 1] = {0};
int ecg_buff_WR_idx = 0;
int ecg_buff_RD_idx = 0;

// circular buffer for input ecg signal
// we need to keep a history of N+1 samples for LP filter
float hp_buff[N + 1] = {0};
int hp_buff_WR_idx = 0;
int hp_buff_RD_idx = 0;

// LP filter outputs a single point for every input point
// This goes straight to adaptive filtering for evaluation
float next_eval_pt = 0;

// running sums for HP and LP filters, values shifted in FILO
float hp_sum = 0;
float lp_sum = 0;

// working variables for adaptive thresholding
float threshold = 0;
boolean triggered = false;
int trig_time = 0;
float win_max = 0;
int win_idx = 0;
int number_iter = 0;

boolean detect(float new_ecg_pt) {
  // copy new point into circular buffer, increment index
  ecg_buff[ecg_buff_WR_idx++] = new_ecg_pt;  
  ecg_buff_WR_idx %= (M+1);

  /* High pass filtering */
  if(number_iter < M){
    // first fill buffer with enough points for HP filter
    hp_sum += ecg_buff[ecg_buff_RD_idx];
    hp_buff[hp_buff_WR_idx] = 0;
  } else{
    hp_sum += ecg_buff[ecg_buff_RD_idx];
    tmp = ecg_buff_RD_idx - M;
    if(tmp < 0){
      tmp += M + 1;
    }
    hp_sum -= ecg_buff[tmp];
    
    float y1 = 0;
    float y2 = 0;
    
    tmp = (ecg_buff_RD_idx - ((M+1)/2));
    if(tmp < 0){
      tmp += M + 1;
    }
    y2 = ecg_buff[tmp];
    y1 = HP_CONSTANT * hp_sum;
    hp_buff[hp_buff_WR_idx] = y2 - y1;
  }
  
  // done reading ECG buffer, increment position
  ecg_buff_RD_idx++;
  ecg_buff_RD_idx %= (M+1);
  
  // done writing to HP buffer, increment position
  hp_buff_WR_idx++;
  hp_buff_WR_idx %= (N+1);
  

  /* Low pass filtering */
  
  // shift in new sample from high pass filter
  lp_sum += hp_buff[hp_buff_RD_idx] * hp_buff[hp_buff_RD_idx];
  
  if(number_iter < N){
    // first fill buffer with enough points for LP filter
    next_eval_pt = 0;
  } else{
    // shift out oldest data point
    tmp = hp_buff_RD_idx - N;
    if(tmp < 0) tmp += (N+1);
    lp_sum -= hp_buff[tmp] * hp_buff[tmp];
    next_eval_pt = lp_sum;
  }
  
  // done reading HP buffer, increment position
  hp_buff_RD_idx++;
  hp_buff_RD_idx %= (N+1);

  /* Adapative thresholding beat detection */
  // set initial threshold        
  if(number_iter < winSize) {
    if(next_eval_pt > threshold) {
      threshold = next_eval_pt;
    }
    number_iter++;
  }
  
  // check if detection hold off period has passed
  if(triggered == true){
    trig_time++;
    if(trig_time >= 100){
      triggered = false;
      trig_time = 0;
    }
  }
  
  // find if we have a new max
  if(next_eval_pt > win_max){
    win_max = next_eval_pt;
  }
  // find if we are above adaptive threshold
  if(next_eval_pt > threshold && !triggered) {
    triggered = true;
    return true;
  }        
  // adjust adaptive threshold using max of signal found 
  // in previous window            
  if(win_idx++ >= winSize){
    // weighting factor for determining the contribution of
    // the current peak value to the threshold adjustment
    float gamma = 0.4;
    // forgetting factor - 
    // rate at which we forget old observations
    // choose a random value between 0.01 and 0.1 for this, 
    float alpha = 0.1 + ( ((float) random(0, RAND_RES) / (float) (RAND_RES)) * ((0.1 - 0.01)));
    
    // compute new threshold
    threshold = alpha * gamma * win_max + (1 - alpha) * threshold;
    
    // reset current window index
    win_idx = 0;
    win_max = -10000000;
  }    
  // return false if we didn't detect a new QRS
  return false; 
}


void setup() {
    Serial.begin(115200);
    initWiFi();
    initFirebase();
}

void loop() {    
  delay(2);
  // Update the time, reset the QRS detection and read the next value
  currentMicros = micros();
  previousMicros = currentMicros;
  boolean QRS_detected = false;
  int next_ecg_pt = analogRead(inputPin);

  /*
  Writing to a CSV file
  dataStr[0] = 0;
  ltoa(millis(), buffer, 10);
  strcat(dataStr, buffer);
  strcat(dataStr, ", ");
  dtostrf(next_ecg_pt, 5, 1, buffer);
  strcat(dataStr, buffer);
  strcat(dataStr, ", ");
  dtostrf(QRS, 5, 1, buffer);
  strcat(dataStr, buffer);
  strcat(dataStr, 0);
  Serial.println(dataStr);
  */

  QRS_detected = detect(next_ecg_pt);
  if(QRS_detected == false) {
    QRS = 0;
  }else if(QRS_detected == true){
    QRS = 1;
    cprTimeRead_2 = cprTimeRead_1;
    cprTimeRead_1 = millis();
    interval = cprTimeRead_1-cprTimeRead_2;
  }
  if(difference > interval){  
    previousMillis = millis();    
  }  
  difference = millis() - previousMillis;

  if (Firebase.ready() && (millis() - sendDataPrevMillis > 3000 || sendDataPrevMillis == 0)){
    if (Firebase.RTDB.setFloat(&fbdo, "test/RRinterval", interval)) {
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }
  }
}  