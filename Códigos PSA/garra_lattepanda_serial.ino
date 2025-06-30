#include <Arduino.h>     // Biblioteca para converter str to float
#include <Servo.h>

//--------------------------------------------------- VARIÁVEIS ------------------------------------------------------------------------
 
int servo_base;       // valor extraído do comando para servo_base
int servo_ombro;      // valor extraído do comando para servo_ombro
int servo_cotovelo;   // valor extraído do comando para extra servo
int servo_camara;
int servo_reset = 0;
int iman_on = 0;

// char tx1, tx2, tx3, ty1, ty2, ty3, auto_mode,
char base, ombro, cotovelo, reset, iman, c1, c2, c3;    // variáveis leitura do código command

float h = 0.115;      // distância das rodas ao centro em mm
float l = 0.32;       // distância entre rodas dianteiras e traseiras em mm

Servo servoBase;    // servo para base (originally servo_angle)
Servo servoOmbro;   // servo para ombro (originally servo1_garra)
Servo servoCotovelo;   // novo servo extra
Servo servoCamara;

const int relePin = 2;

// Global variables for tracking current servo positions
int currentBasePulse = 1500;   // posição inicial neutra para servo_base
int currentOmbroPulse = 1500;  // posição inicial neutra para servo_ombro
int currentCotoveloPulse = 1500;  // posição inicial neutra para servo extra

// Define increments and limits para segurança
int stepBase = 5;      
int maxBasePulse = 2500;
int minBasePulse = 500;

int stepOmbro = 2;     
int maxOmbroPulse = 2000;  //1700
int minOmbroPulse = 1000;  //1300

int stepCotovelo = 2;     
int maxCotoveloPulse = 2500;
int minCotoveloPulse = 500;

int stepCamara = 2;     
int maxCamara = 120;
int minCamara = 60;
int angleCamara = 90;

String command;
String unoCommand;
String commandUno;
int readyup = 0;

void setup() {
    pinMode(relePin, OUTPUT);
    Serial.begin(115200);
    //Serial1.begin(9600);
    servoBase.attach(11);     // Use pin 3 para o servo base
    servoOmbro.attach(13);   // Use pin 5 para o servo ombro
    servoCotovelo.attach(6);    // Use pin 9 para o novo servo extra (ajuste se necessário)
    servoCamara.attach(5);
    delay(20);
    servoBase.writeMicroseconds(1500);
    servoOmbro.writeMicroseconds(1500);
    servoCotovelo.writeMicroseconds(1500);
    servoCamara.write(angleCamara);
    delay(20);
}

void loop() {
    read_command();

    process_command();

    //read_uno();

    // send_command();

    if (servo_reset == 1) {
        currentBasePulse = 1500;
        currentOmbroPulse = 1500;
        currentCotoveloPulse = 1500;
    } 
    else {
        // Atualiza servo_base
        if (servo_base > 1) {
            currentBasePulse += stepBase;
            if (currentBasePulse > maxBasePulse) {
                currentBasePulse = maxBasePulse;
            }
        } 
        else if (servo_base < 1) {
            currentBasePulse -= stepBase;
            if (currentBasePulse < minBasePulse) {
                currentBasePulse = minBasePulse;
                }
        }

        // Atualiza servo_ombro
        if (servo_ombro > 1) {
            currentOmbroPulse += stepOmbro;
            if (currentOmbroPulse > maxOmbroPulse) {
                currentOmbroPulse = maxOmbroPulse;
            }
        } else if (servo_ombro < 1) {
            currentOmbroPulse -= stepOmbro;
            if (currentOmbroPulse < minOmbroPulse) {
                currentOmbroPulse = minOmbroPulse;
            }
        }

        // Atualiza servo extra
        if (servo_cotovelo > 1) {
            currentCotoveloPulse += stepCotovelo;
            if (currentCotoveloPulse > maxCotoveloPulse) {
                currentCotoveloPulse = maxCotoveloPulse;
            }
        } else if (servo_cotovelo < 1) {
            currentCotoveloPulse -= stepCotovelo;
            if (currentCotoveloPulse < minCotoveloPulse) {
                currentCotoveloPulse = minCotoveloPulse;
            }
        }

        angleCamara = converteAngulo(servo_camara);        

        if (iman_on == 1) {
          digitalWrite(relePin, HIGH);
        }
        else if (iman_on == 0) {
          digitalWrite(relePin, LOW);
        }
    }
           
    // Atualiza os servos com os pulsos calculados
    servoBase.writeMicroseconds(currentBasePulse);
    servoOmbro.writeMicroseconds(currentOmbroPulse);
    servoCotovelo.writeMicroseconds(currentCotoveloPulse);
    servoCamara.write(angleCamara);

    delay(20);
}

void read_command(){
    if (Serial.available() > 0){
    command = "";
        for (int i = 0; i < 15; i++) {
            command += (char)Serial.read();
        }
    }
    while (Serial.available()) Serial.read();  // Descarta qualquer lixo restante
}

void process_command(){

    base = command.charAt(7); ombro = command.charAt(8); cotovelo = command.charAt(9); reset = command.charAt(10); iman = command.charAt(11);

    servo_base    = (base - '0');       // Valor de 0, 1 ou 2
    servo_ombro   = (ombro - '0');
    servo_cotovelo = (cotovelo - '0');
    servo_reset   = (reset - '0');      // Valor de 0 ou 1
    iman_on       = (iman - '0');       // Valor de 0 ou 1

    c1 = command.charAt(12); c2 = command.charAt(13); c3 = command.charAt(14);
    servo_camara = (c1 - '0') * 100 + (c2 - '0') * 10 + (c3 - '0');   // Valor de 100 a 300

    unoCommand = command;
    unoCommand.remove(7,8); // Remove 8 carateres a partir do 8º (sobram 7)   
}

void read_uno(){
    if (Serial1.available() > 0){
    commandUno = (char)Serial1.read();
    readyup = commandUno.charAt(0) - '0';
    }
    while (Serial1.available()) Serial1.read();  // Descarta qualquer lixo restante
}

void send_commandUno(){
  if (readyup == 1){
    Serial.print(commandUno);
  }
}

void send_command(){
  //if (readyup == 1){
    Serial1.print(unoCommand);
  //}
}

int converteAngulo(int valor) {
  return map (valor, 100, 300, 50, 130);
}
