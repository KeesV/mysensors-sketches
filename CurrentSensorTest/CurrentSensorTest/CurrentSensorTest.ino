int calVal = 512;
int mVpA = 61;

void setup() {
  Serial.begin(9600);
}

void loop() {
  int val = analogRead(A5);
  int valAdj = val - calVal;
  float milliVolts = ((valAdj * 5.00) / 1024) * 1000;
  float Amps = milliVolts / mVpA;
  Serial.println(val);
  Serial.println(milliVolts);
  Serial.println(Amps);
  delay(500);
}
