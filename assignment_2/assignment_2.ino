/*
 * MUSI 6304 - Project Studio
 * Assignment 2 - Arduino
 *    Ryan Rose
 *   
 *   HC-SR04 Distance Sensing Code from 
 *   Dejan Nedelkovski at www.HowToMechatronics.com
 */
#include <CircularBuffer.h>

// Pin definitions
#define TRIG 13
#define ECHO 12
#define RED 11
#define GREEN 9
#define BLUE 10
#define BTN0 7
#define BTN1 6
#define BTN2 5
#define BTN3 4
#define POT 0

// Thresholds, data pruning constants
#define DISTANCE_MAX 50
#define DISTANCE_HISTORY 10
#define PEAK_THRESHOLD 2
#define PLAYRATE_LOOKBACK 1

// Playing state
#define PLAYING 1
#define NOT_PLAYING 0

void setup() {
  Serial.begin(9600);

  pinMode(TRIG, OUTPUT); // Ultrasound output
  pinMode(ECHO, INPUT); // Ultrasound input

  pinMode(RED, OUTPUT); // Red output
  pinMode(BLUE, OUTPUT); // Red output
  pinMode(GREEN, OUTPUT); // Red output

  pinMode(BTN0, INPUT);
  pinMode(BTN1, INPUT);
  pinMode(BTN2, INPUT);
  pinMode(BTN3, INPUT);
}

float bufferAverage(CircularBuffer<int,DISTANCE_HISTORY> b, int i1, int i2) {
  float sum = 0.0;
  for(int i = i1; i < i2; i++) {
    sum += b[i];
  }
  return sum / (float)(i2 - i1);
}

/*
 * getCurrentDistance()
 *    Measures the distance from the ultrasound sensor.
 *    Output: sets `distance`
 */
int measuredDistance = 0;
long duration = 0.0;
CircularBuffer<int,DISTANCE_HISTORY> distanceMeasurements;
CircularBuffer<float,DISTANCE_HISTORY> avgDistanceMeasurements;
CircularBuffer<unsigned long,DISTANCE_HISTORY> distanceTimestamps;
CircularBuffer<unsigned long,DISTANCE_HISTORY> avgDistanceTimestamps; 

void getCurrentDistance() {
  digitalWrite(TRIG, LOW); // Turn off ultrasound signal
  delayMicroseconds(2); // Ensure cleared
  digitalWrite(TRIG, HIGH); // Send 10µs of ultrasound signal
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW); // Turn off ultrasound signal
  duration = pulseIn(ECHO, HIGH); // Reads ultrasound, returns travel time in µs
  measuredDistance = duration * 0.034 / 2; // Cal`culating the distance
  if(measuredDistance < DISTANCE_MAX) {
    distanceMeasurements.push(measuredDistance);
    distanceTimestamps.push(micros());
  }
  else {
    distanceMeasurements.push(DISTANCE_MAX);
    distanceTimestamps.push(micros());
  }
}

/*
 * determinePlayState()
 *    Determines whether or not to play the instrument based on distance readings.
 *    Output: sets `playState`
 */
bool playState = NOT_PLAYING;
float previous;
float current;
int repeatCtr = 0;
float peak = 50.0;
float playRate;

void determinePlayState() {
  current = bufferAverage(distanceMeasurements, 0, distanceMeasurements.size());
  avgDistanceMeasurements.push(current);
  avgDistanceTimestamps.push(micros());
 
  // When motion moves up, stop playing and force "breath"
  if((current - peak) > PEAK_THRESHOLD || current == 50.0){
    repeatCtr = 0;
    playState = NOT_PLAYING;
    playRate = 0.0;
    if(current == 50.0) peak = 50.0; 
  }
  // If not moving, force "breath"
//  else if(repeatCtr > 5) { 
//    peak = 0.0; 
//    playRate = 0.0;
//  }
  // If moving, update peak
  else {
    playState = PLAYING; 
    if(current < peak) peak = current;
  }

  if(previous == current) repeatCtr++;
  previous = current;
  
}


void determinePlayRate() {
  float currentValue = avgDistanceMeasurements.last();
  float currentTimestamp = avgDistanceTimestamps.last();
  float lookbackIndex = max(0, avgDistanceMeasurements.size() - 1 - PLAYRATE_LOOKBACK);
  float lookbackValue = avgDistanceMeasurements[lookbackIndex];
  float lookbackTimestamp = avgDistanceTimestamps[lookbackIndex];

  float playRateRaw = max(0, (lookbackValue - currentValue) / (currentTimestamp - lookbackTimestamp));
  if(playRateRaw * 100000 < 10.0) playRate = playRateRaw;
}

int potSlider = 200;
int root;
int readPot;
void getRoot() {
  readPot = analogRead(0);
  if(readPot > 200) {
    potSlider = readPot;
  }
  root = map(potSlider, 200, 1023, 0, 12);
}

void loop() {  
  getCurrentDistance();
  determinePlayState();
  if(playState == PLAYING) {
    determinePlayRate();
  }
  getRoot();
  
  Serial.print(playState);
  Serial.print(" ");
  Serial.print(playRate * 100000);
  Serial.print(" ");
  Serial.print(digitalRead(BTN0));
  Serial.print(" ");
  Serial.print(digitalRead(BTN1));
  Serial.print(" ");
  Serial.print(digitalRead(BTN2));
  Serial.print(" ");
  Serial.print(digitalRead(BTN3));
  Serial.print(" ");
  Serial.println(root);
}
