#include <Servo.h>
#include <Arduino.h>

// Sensor pins
const int infraLeft  = 12;
const int infraRight = 13;

// Ultrasound pins
const int trigUltra = 3;
const int echoUltra = 2;
float duration, distance;   // Variables

// Left wheel motor pins
const int dirLeft = 7;        // Controle de direção roda esquerda
const int pwmLeft = 6;        // Controle de velocidade roda esquerda (PWM)
const int encLeft = 4;

// Right wheel motor pins
const int dirRight = 11;      // Controle de direção roda direita
const int pwmRight = 10;      // Controle de velocidade roda direita (PWM)
const int encRight = 8;


// Steering servo pin (front wheels) - opcional se usar direção diferencial
const int pinServoDir = 9;
Servo servoDir;
// Ultrasound servo
const int pinServoUltra = 5;
Servo servoUltra;

// Steering angles
const int centerAngle = 90;  // Straight ahead
const int leftAngle = 75;    // Turn left
const int rightAngle = 105;  // Turn right

// Motor speeds (0-255)
const int normalSpeed = 200;  // Velocidade normal
const int turnSpeed = 200;    // Velocidade para curvas
const int slowSpeed = 200;    // Velocidade lenta para roda interna em curvas
int speed = 200;

// Variáveis
int distLimite = 40;  // Parar quando a distância for 30 cm
int distFront, distLeft, distRight;
int angle;  // Variável para o ângulo do servo
bool isWall = false;  // Variável para indicar se é parede ou obstáculo
int farReadings = 0;  // Variável para contagem de leituras distantes

// Variáveis para encoders e desvio
int pulsosLeft = 0, pulsosRight = 0;
int pulsosPorVolta = 270;  // Ajustar conforme teu robô

// Variáveis comando
String command;
int eixo_x = 200; int eixo_y = 200; int auto_mode = 0; // variáveis que recebem o valor da posição eixo x e do eixo y do analógico
char tx, tx1, tx2, tx3, ty, ty1, ty2, ty3;             // variáveis leitura do código command
int y = 255; int x = 1570; int y_m1 = 255; int y_m2 = 255;
bool rot_m1 = HIGH; bool rot_m2 = LOW;             // variáveis para direção dos motores
float h = 0.115;                      // distânica das rodas ao centro em mm
float l = 0.32;                       // distância entre rodas dianteiras e traseiras em mm
float enc_m1; float enc_m2;          // velocidade em RPM dos encoders

void setup() {
  Serial.begin(112500);

  pinMode(infraLeft  , INPUT);
  pinMode(infraRight , INPUT);
  pinMode(trigUltra  , OUTPUT);
  pinMode(echoUltra  , INPUT);
  pinMode(dirLeft    , OUTPUT);
  pinMode(pwmLeft    , OUTPUT);
  pinMode(dirRight   , OUTPUT);
  pinMode(pwmRight   , OUTPUT);
  pinMode(encLeft    , INPUT_PULLUP);
  pinMode(encRight   , INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(encLeft)  , contarPulsoLeft  , RISING);
  attachInterrupt(digitalPinToInterrupt(encRight) , contarPulsoRight , RISING);

  
  // Initialize steering servo (opcional se usar apenas direção diferencial)
  servoDir.attach(pinServoDir);
  servoDir.write(centerAngle);  // Start with wheels straight
  servoUltra.attach(pinServoUltra);
  servoDir.write(90);     // Centrar o servo  

  /*while (true) {
      analogWrite(pwmLeft, 255);
      analogWrite(pwmRight, 255);
      digitalWrite(dirLeft, LOW);
      digitalWrite(dirRight, LOW);
      if (server.available() > 0) {
        break;
      }
  }*/
}

void loop() {
  
  if (Serial.available() >= 7){
    command = "";
    for (int i = 0; i < 7; i++){
      command += (char)Serial.read();
    }
  }
  while (Serial.available() > 0) {
    Serial.read();
  }

  process_command();
  
  if (auto_mode == 1){
    codeAuto();
  }
  else {
    codeManual();
  }
  Serial.write(0x06);
}

void read_command(){
    if (Serial.available() >= 7){
    command = "";
        for (int i = 0; i < 7; i++) {
            command += (char)Serial.read();
        }
    }
    while (Serial.available()) Serial.read();  // Descarta qualquer lixo restante
}

void process_command(){

  tx1 = command.charAt(0); tx2 = command.charAt(1); tx3 = command.charAt(2);  // Aquisição dos caratéres na posição 0, 1 e 2 da mensagem
  eixo_x = (tx1 - '0') * 100 + (tx2 - '0') * 10 + (tx3 - '0');

  ty1 = command.charAt(3); ty2 = command.charAt(4); ty3 = command.charAt(5);  // Igual acima mas para eixo y
  eixo_y = (ty1 - '0') * 100 + (ty2 - '0') * 10 + (ty3 - '0');

  auto_mode = (command.charAt(6) - '0');

  Serial.print(eixo_x); Serial.print(eixo_y); Serial.println(auto_mode);
}

//--------------------------------------------------------------

int converteParaPWM(int valor) {
  if (valor < 200) {
    return map(valor, 100, 200, 0, 255);
  }
  else if (valor > 200) {
    return map(valor, 200, 300, 255, 0);
  }
}

int convertePosicao(int valor) {
  if (valor < 200) {
    return map(valor, 100, 200, 60, 90);
  }
  else if (valor > 200) {
    return map(valor, 200, 300, 90, 120);
  }
}

int converteAngulo(int valor) {
  return map (valor, 100, 300, -30, 30);
}

int diff(int ang, int pwm) {
  float r;
  float a;
  float vel_m1; // Velocidade desejada m1
  float vel_m2; // Velocidade desejada m2

  if (ang == 0)  {
	  y_m1 = pwm;
    y_m2 = pwm;	
	}

  if (ang < 0)   {
    vel_m1 = enc_m1; // Velocidade do motor 1 é para manter
    y_m1 = pwm;

    a = -ang * PI / 180;
    r = l / tan(a);
    vel_m2 = vel_m1 / (r + h) * (r - h); // Diferencial para velocidade desejada teórica

    if (enc_m2 > enc_m1) {  
      y_m2 = y_m1;           // If para prevenir mudanças bruscas de velocidade
    }

    if (y_m1 == 255) {
      y_m2 = y_m1;
    }
    else if (enc_m2 > vel_m2)  {
      y_m2 = y_m2 + 5;
    }
    else if (enc_m2 < vel_m2) {
      y_m2 = y_m2 - 5;
    }
    else if (enc_m2 = vel_m2) {
      y_m2 = y_m2;
    }
  }

  if (ang > 0)    {
    vel_m2 = enc_m2;
    y_m2 = pwm;

    a = ang * PI / 180;
    r = l / tan(a);
    vel_m1 = vel_m2 / (r + h) * (r - h);

    if (enc_m1 > enc_m2) {
      y_m1 = y_m2;
    }

    if (y_m2 == 255) {
      y_m1 = y_m2;
    }
    else if (enc_m1 > vel_m1)  {
      y_m1 = y_m1 + 5;
    }
    else if (enc_m1 < vel_m1) {
      y_m1 = y_m1 - 5;
    }
    else if (enc_m1 = vel_m1) {
      y_m1 = y_m1;
    }
  }
}

void readSpeed()  {

  int i1 = 0;          // variável para leitura pulsos motor 1
  int i2 = 0;          // variável para leitura pulsos motor 2

  i1 += pulseIn(8, HIGH, 50000); //SIGNAL OUTPUT PIN 9 with  white line,cycle = 2*i,1s = 1000000us，Signal cycle pulse number：27*2
  i2 += pulseIn(4, HIGH, 50000); //500000 significa que se não receber pulso durante 0.5s retorna 0
  
  //speed   r/min  (60*1000000/(45*6*2*i))  
  enc_m1 = (111111 / i1); // '2' ajusta valor de i porque só conta HIGH
  enc_m2 = (111111 / i2); // i*2 ajusta para HIGH's e LOW's do encoder

  i1 = 0;
  i2 = 0;
}


void codeManual(){
  // Controlo Motores
    if (eixo_y < 200) {
      y = converteParaPWM(eixo_y);
      //Serial.println("Movendo para frente");  
      rot_m1 = LOW; rot_m2 = HIGH;
    }
    else if (eixo_y > 200) {
      y = converteParaPWM(eixo_y);
      //Serial.println("Movendo para trás");
      rot_m1 = HIGH; rot_m2 = LOW; 
    }
    else   {
      y = 255;
      // rot_m1 = LOW; rot_m2 = HIGH;
    }

      
    if (eixo_x != 200){
      x = convertePosicao(eixo_x);
    }                       // Controlo Direção
    else{
      x = 90;
    }

    //readSpeed(); // leitura das velocidades de rotação dos motores
    //diff(converteAngulo(eixo_x), y); // chamada da função diff para ajustar velocidades 
  
  analogWrite(pwmRight, y);  //y_m1
  analogWrite(pwmLeft, y);   //y_m2
  digitalWrite(dirRight, rot_m1);
  digitalWrite(dirLeft, rot_m2);
  servoDir.write(x);
}

//------------------------------------------------------------------------
void moveForward() {
  // Ambas as rodas para frente com mesma velocidade
  speed = 200;
  servoDir.write(centerAngle);         // Keep wheels straight (se usar servo)
}

void turnLeft() {
  // Direção diferencial: roda direita mais rápida que a esquerda
  speed = 200;
  servoDir.write(leftAngle);           // Turn front wheels left (se usar servo)
}

void turnRight() {
  // Direção diferencial: roda esquerda mais rápida que a direita
  speed = 200;
  servoDir.write(rightAngle);          // Turn front wheels right (se usar servo)
}

void stopMotors() {
  // Para ambas as rodas
  speed = 255;
  servoDir.write(centerAngle);         // Return wheels to center (se usar servo)
}

void aplicarMovimento() {
  // Centraliza todo o controlo dos motores
  digitalWrite(dirLeft, LOW);
  digitalWrite(dirRight, HIGH);
  analogWrite(pwmLeft, speed);
  analogWrite(pwmRight, speed);
}

float measure() {
  digitalWrite(trigUltra, LOW);  
	delayMicroseconds(2);  
	digitalWrite(trigUltra, HIGH);  
	delayMicroseconds(10);  
	digitalWrite(trigUltra, LOW);  
  duration = pulseIn(echoUltra, HIGH);
  distance = (duration*.0343)/2;
  delay(20);
  return distance; 
}

bool checkFront() {  // Verifica dist frontal, TRUE se inferior ao distLimite
  servoUltra.write(90);  // Apontar sensor para frente
  distFront = measure();
  if (distFront < distLimite) {
    return true;
  }
  else {
    return false;
  }
}


bool scan() {
  // Array para armazenar as distâncias em diferentes ângulos
  float distances[3];  
  int angles[3] = {35, 90, 145};  // Direita, Centro, Esquerda
  int closeReadings = 0;  // Contador de leituras próximas
  farReadings = 0;  // Reset do contador de leituras distantes
  
  Serial.println("=== INICIANDO SCAN ===");
  
  // Fazer o scan em 3 posições
  for(int i = 0; i < 3; i++) {
    servoUltra.write(angles[i]);
    delay(2000);  // Aguardar o servo se posicionar
    distances[i] = measure();
    
    Serial.print("Ângulo ");
    Serial.print(angles[i]);
    Serial.print("°: ");
    Serial.print(distances[i]);
    Serial.println(" cm");
    
    // Contar quantas leituras estão próximas (dentro do limite)
    if(distances[i] < 200 && distances[i] > 2) {
      closeReadings++;
    }
    
    // Contar quantas leituras estão distantes (fora do limite)
    if(distances[i] > 200) {
      farReadings++;
    }
  }
  
  // Atualizar variáveis globais
  distRight = distances[0];  // 45°
  distFront = distances[1];  // 90°
  distLeft = distances[2];   // 135°

  Serial.print("Distâncias: ");
  Serial.print("Direita: ");
  Serial.print(distRight);
  Serial.print(" cm, Centro: ");
  Serial.print(distFront);
  Serial.print(" cm, Esquerda: ");
  Serial.print(distLeft);
  Serial.println(" cm");  
  
  // Análise: distinguir parede de obstáculo
  if(farReadings >= 1) {
    isWall = false;  // Se há pelo menos uma leitura longe, não é parede
  } else if(closeReadings >= 2) {
    isWall = true;   // Se múltiplas leituras próximas e nenhuma distante - é parede
  } else if(closeReadings == 1) {
    isWall = false;  // Uma leitura próxima apenas - provável obstáculo
  } else {
    isWall = false;
  }
  
  // Retornar servo para posição central
  servoUltra.write(90);
  delay(300);
  
  Serial.println("=== FIM DO SCAN ===");
  
  // Retorna true se for PAREDE, false se for OBSTÁCULO
  return isWall;
}

void contarPulsoLeft() {
  pulsosLeft++;
}

void contarPulsoRight() {
  pulsosRight++;
}

void desviarObstaculo() {
  Serial.println("=== DESVIO DE OBSTÁCULO ===");
  
  // 1. Decidir direção (direita se esquerda mais livre)
  bool irDireita = (distLeft > distRight);
  int anguloDesvio = irDireita ? (rightAngle + 15) : (leftAngle - 15);
  
  Serial.println(irDireita ? "-> Direita" : "-> Esquerda");
  

    // Reset rápido dos motores
  digitalWrite(dirLeft, LOW);
  digitalWrite(dirRight, HIGH);
  analogWrite(pwmLeft, 240);
  analogWrite(pwmRight, 240);
  analogWrite(pwmLeft, 255);
  analogWrite(pwmRight, 255);
  
  // 2. Recuar usando encoders em vez de delay
  Serial.println("Recuando...");
  pulsosLeft = 0;  // Reset do contador
  while(pulsosLeft < pulsosPorVolta/2) {  // Recuar 1/4 de volta
    digitalWrite(dirLeft, HIGH);    // Ambos para trás
    digitalWrite(dirRight, LOW);
    analogWrite(pwmLeft, normalSpeed);
    analogWrite(pwmRight, normalSpeed);
    contarPulsoLeft();  // Contar pulsos do encoder
    delay(50);  // Pequeno delay para estabilidade
  }
  
  // Parar após recuar
  analogWrite(pwmLeft, 255);
  analogWrite(pwmRight, 255);
  delay(200);
  
  // 3. Executar desvio
  servoDir.write(anguloDesvio);    // Virar rodas
  delay(300);
  
  // 4. Avançar até caminho ficar livre
  pulsosRight = 0;
  while(pulsosRight < 1 * pulsosPorVolta) {
    digitalWrite(dirLeft, LOW);
    digitalWrite(dirRight, HIGH);
    analogWrite(pwmLeft, normalSpeed);
    analogWrite(pwmRight, normalSpeed);
    contarPulsoRight();  // Contar pulsos do encoder
    delay(100);  // Os encoders contam automaticamente via interrupção
  }
  
  // 6. Virar para o lado oposto para voltar ao caminho
  Serial.println("Virando para voltar ao caminho...");
  int anguloRetorno = irDireita ? (leftAngle - 15) : (rightAngle + 15);  // Oposto da direção inicial
  servoDir.write(anguloRetorno);
  delay(300);
  
  // 7. Primeiro ciclo: Avançar com rodas ainda viradas
  Serial.println("Avançando com rodas viradas...");
  pulsosRight = 0;
  while(pulsosRight < 2.3*pulsosPorVolta) {  // Meio volta com rodas viradas
    digitalWrite(dirLeft, LOW);
    digitalWrite(dirRight, HIGH);
    analogWrite(pwmLeft, normalSpeed);
    analogWrite(pwmRight, normalSpeed);
    contarPulsoRight();  // Contar pulsos do encoder
    delay(50);
  }
  
  // 8. Centrar rodas para suavizar ângulo de aproximação
  Serial.println("Centrando rodas para suavizar aproximação...");
  servoDir.write(centerAngle);  // Centrar rodas para ângulo suave
  delay(300);
  
  // 9. Segundo ciclo: Avançar com rodas centradas até encontrar a linha
  Serial.println("Avançando com rodas centradas até encontrar linha...");
  pulsosRight = 0;
  while(pulsosRight < pulsosPorVolta) {  
    digitalWrite(dirLeft, LOW);
    digitalWrite(dirRight, HIGH);
    analogWrite(pwmLeft, normalSpeed);
    analogWrite(pwmRight, normalSpeed);
    delay(50);
    
    // Verificar se encontrou a linha branca
    int leftSensor = digitalRead(infraLeft);
    int rightSensor = digitalRead(infraRight);
    
    if (leftSensor == LOW || rightSensor == LOW) {
      Serial.println("Linha branca detectada!");
      break;  // Sair do loop quando encontrar a linha
    }
  }
  
  // 9. Viragem final para alinhar com a linha
  Serial.println("Alinhando com a linha...");
  servoDir.write(anguloDesvio);  // Virar na mesma direção do desvio inicial
  
  // 10. Avanço curto para ficar centrado na linha
  pulsosRight = 0;
  while(pulsosRight < pulsosPorVolta/4) {  // Movimento muito curto para centrar
    digitalWrite(dirLeft, LOW);
    digitalWrite(dirRight, HIGH);
    analogWrite(pwmLeft, normalSpeed);
    analogWrite(pwmRight, normalSpeed);
    contarPulsoRight();  // Contar pulsos do encoder
    delay(50);
  }
  
  // 11. Centrar rodas para seguir linha
  servoDir.write(centerAngle);
  delay(200);
  


  // Reset rápido dos motores
  digitalWrite(dirLeft, HIGH);
  digitalWrite(dirRight, LOW);
  analogWrite(pwmLeft, 240);
  analogWrite(pwmRight, 240);
  analogWrite(pwmLeft, 255);
  analogWrite(pwmRight, 255);
  
  Serial.println("=== DESVIO CONCLUÍDO ===");
}

void codeAuto() {
  // Verificar obstáculos à frente
  if (checkFront()) {
    Serial.println("Obstáculo detectado - iniciando procedimento...");
    
    // Parar e fazer scan
    stopMotors();
    aplicarMovimento();
    delay(300);
    
    // Reset das rodas após paragem
    digitalWrite(dirLeft, HIGH);    // Pulso para trás
    digitalWrite(dirRight, LOW);
    analogWrite(pwmLeft, 240);
    analogWrite(pwmRight, 240);
    analogWrite(pwmLeft, 255);     // Parar
    analogWrite(pwmRight, 255);
    
    scan();  // Analisa ambiente
    desviarObstaculo();  // Executa desvio
    return;
  }
  
  // Seguir linha normalmente
  int leftSensor = digitalRead(infraLeft);
  int rightSensor = digitalRead(infraRight);
  
  if (leftSensor == HIGH && rightSensor == HIGH) {
    moveForward();
  } else if (leftSensor == LOW && rightSensor == HIGH) {
    turnRight();
  } else if (leftSensor == HIGH && rightSensor == LOW) {
    turnLeft();
  } else {
    stopMotors();
  }
  
  aplicarMovimento();
}
