#include "AnalogIn.h"
#include "PwmOut.h"
#include "ThisThread.h"
#include "mbed.h"


// motors
DigitalOut in1(D2); // left motor spinning direction
DigitalOut in2(D4); // left motor spinning direction
PwmOut    enA(D3); // left motor speed

DigitalOut in3(D5); // right motor spinning direction
DigitalOut in4(D7); // right motor spinning direction
PwmOut    enB(D6); // right motor speed

// ir sensors
AnalogIn ir_L(A0); // left sensor
AnalogIn ir_M(A1); // middle sensor
AnalogIn ir_R(A2); // right sensor

//  pump
DigitalOut pump(D8); 

// speaker 
PwmOut speaker(D9);

// servo
PwmOut servo(D10); 

// speed and pwm config
float duty = 160.0f / 255.0f; // speed 63%

// motor control functions
void forward(){
    in1 = 1; in2 = 0; // left motor forward
    in3 = 0; in4 = 1; // right motor forward
    enA.write(duty); // apply speed
    enB.write(duty); // apply speed
}

void backward(){ // might not use
    in1 = 0; in2 = 1; // left motor backward 
    in3 = 1; in4 = 0; // right motor backward
    enA.write(duty); // apply speed
    enB.write(duty); // apply speed
}

void turnLeft(){
    in1 = 1; in2 = 0; // left motor forward
    in3 = 1; in4 = 0;  // right motor backward
    enA.write(duty); // apply speed
    enB.write(duty); // apply speed
}

void turnRight(){
    in1 = 0; in2 = 1; // left motor backward
    in3 = 0; in4 = 1;  // right motor forward
    enA.write(duty); // apply speed
    enB.write(duty); // apply speed
}

void stop(){
    in1 = 0; in2 = 0; // stop left motor
    in3 = 0; in4 = 0; // stop right motor
}

 // convert angle (0-180) to PWM pulse width (all the way left(1ms) to middle(2ms) over 20ms period)
void setServoAngle(int angle) {
    float pulseWidth = 0.001 + ((float)angle / 180.0f) * 0.001; // 1ms + (angle/180)*1ms
    servo.pulsewidth(pulseWidth); 
}

// pump control w servo function
void extinguish() {
    printf("Extinguishing with sweep...\n");
    pump = 1;

        // sweep from 0° to 90° 
        for (int angle = 0; angle <= 90; angle += 5) {
            setServoAngle(angle);
            ThisThread::sleep_for(100);
        }

        // sweep back from 90° to 0°
        for (int angle = 90; angle >= 0; angle -= 5) {
            setServoAngle(angle);
            ThisThread::sleep_for(100);
        }

    pump = 0;

    // confirmation beep
    speaker.period(1.0 / 500);
    speaker.write(0.2);
    ThisThread::sleep_for(300);
    speaker.write(0.0);
}

int main() {
    // set PWM period and duty cycle 
    enA.period_ms(1); 
    enB.period_ms(1);
    servo.period(0.02); // 20ms standard for servos

    while(1){
        
        // read from the three sensors 
        float s1 = ir_R.read() * 1023; // convert analog to 10-bit ADC
        float s2 = ir_M.read() * 1023; // convert analog to 10-bit ADC
        float s3 = ir_L.read() * 1023; // convert analog to 10-bit ADC

        // configuring speaker based on sensor readings
        float avgFire = (s1 + s2 + s3) / 3.0; // averaging fire intensity
        float freq = 500 + (3500 * (1.0 - avgFire / 1023.0)); // 500Hz to 4000Hz range
        speaker.period(1.0 / freq);
        speaker.write(0.1); // gentle tone as it approaches

        // fire detection conditions
        if (s1 < 350) { // fire detected on the right
            turnRight(); // face fire
            ThisThread::sleep_for(400); // wait to turn
            stop(); 
            forward(); // move forward after turning
            ThisThread::sleep_for(500);
            stop();
            // activate pump & servo 
            extinguish();
        } else if (s2 < 400) { // fire detected in front
            forward();
            ThisThread::sleep_for(500);
            stop();
            // activate pump & servo 
            extinguish();
        } else if (s3 < 350) { // fire detected on the left
            turnLeft(); // face fire
            ThisThread::sleep_for(400); // wait to turn
            stop(); 
            forward(); // move forward after turning
            ThisThread::sleep_for(500);
            stop();
            // activate pump & servo 
            extinguish();
        } else { // no fire detected
            stop(); // idle state
            speaker.write(0.0); // quiet if no fire detected 
        }

        ThisThread::sleep_for(10); // small delay
    }
