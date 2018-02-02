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

// Thresholds, data pruning constants
#define DISTANCE_MAX 50
#define DISTANCE_HISTORY 10
#define PEAK_THRESHOLD 2
#define PLAYRATE_LOOKBACK 4

// Playing state
#define PLAYING 1
#define NOT_PLAYING 0

void setup() {
  Serial.begin(9600);

  pinMode(TRIG, OUTPUT); // Ultrasound output
  pinMode(ECHO, INPUT); // Ultrasound input
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
CircularBuffer<unsigned long,DISTANCE_HISTORY> distanceTimestamps; 

void getCurrentDistance() {
  digitalWrite(TRIG, LOW); // Turn off ultrasound signal
  delayMicroseconds(2); // Ensure cleared
  digitalWrite(TRIG, HIGH); // Send 10µs of ultrasound signal
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW); // Turn off ultrasound signal
  duration = pulseIn(ECHO, HIGH); // Reads ultrasound, returns travel time in µs
  measuredDistance = duration * 0.034 / 2; // Calculating the distance
  if(measuredDistance < DISTANCE_MAX) {
    distanceMeasurements.push(measuredDistance);
    distanceTimestamps.push(millis());
  }
  else {
    distanceMeasurements.push(DISTANCE_MAX);
    distanceTimestamps.push(millis());
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
 
  // When motion moves up, stop playing and force "breath"
  if((current - peak) > PEAK_THRESHOLD || current == 50.0){
    repeatCtr = 0;
    playState = NOT_PLAYING;
    if(current == 50.0) { 
      peak = 50.0; 
      playRate = 0;
    }
  }
  // If not moving, force "breath"
  else if(repeatCtr > 3) { 
    peak = 0.0; 
    playRate = 0;
  }
  // If moving, update peak
  else {
    playState = PLAYING; 
    if(current < peak) peak = current;
  }

  if(previous == current) repeatCtr++;
  previous = current;
}


void determinePlayRate() {
  float currentValue = distanceMeasurements.last();
  float currentTimestamp = distanceTimestamps.last();
  float lookbackIndex = max(0, distanceMeasurements.size() - PLAYRATE_LOOKBACK);
  float lookbackValue = distanceMeasurements[lookbackIndex];
  float lookbackTimestamp = distanceTimestamps[lookbackIndex];

  playRate = max(0, (lookbackValue - currentValue) / (currentTimestamp - lookbackTimestamp));
}

void loop() {
  getCurrentDistance();
  determinePlayState();
  if(playState == PLAYING) {
    determinePlayRate();
  }
  Serial.print(playState);
  Serial.print(" : ");
  Serial.print(current); 
  Serial.print(", peak: "); 
  Serial.print(peak);
  Serial.print(", rate: "); 
  Serial.println(playRate); 
}