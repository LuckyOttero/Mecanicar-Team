#include <Servo.h> // Biblioteca para controlo de servos
#include <Arduino.h> // Biblioteca base do Arduino

// Pinos do sensor infravermelho
const int infraLeft  = 12;
const int infraRight = 13;

// Pinos do sensor ultrassom
const int trigUltra = 3;
const int echoUltra = 2;
float duration, distance;   // Variables

// Pinos do motor da roda esquerda
const int dirLeft = 7;        // Controle de direção roda esquerda
const int pwmLeft = 6;        // Controle de velocidade roda esquerda (PWM)
const int encLeft = 4;

// Pinos do motor da roda direita
const int dirRight = 11;      // Controle de direção roda direita
const int pwmRight = 10;      // Controle de velocidade roda direita (PWM)
const int encRight = 8;


// Pino do servo de direção (rodas dianteiras)
const int pinServoDir = 9;
Servo servoDir;
// Pino de servo do sensor ultrassom
const int pinServoUltra = 5;
Servo servoUltra;

// Ângulos do servo de direção para o modo automático
const int centerAngle = 90;  // Em frente
const int leftAngle = 75;    // Esquerda
const int rightAngle = 105;  // Direita

// Motor speeds (0-255)
const int normalSpeed = 160;  // Velocidade normal
const int turnSpeed = 200;    // Velocidade para curvas
const int slowSpeed = 200;    // Velocidade lenta para roda interna em curvas
int speed = 160;

// Variáveis
int distLimite = 30;  // Parar quando a distância for 30 cm
int distFront, distLeft, distRight; // Distâncias medidas pelo sensor ultrassom
bool isWall = false;  // Variável para indicar se é parede ou obstáculo

// Variáveis para encoders e desvio
int pulsosLeft = 0, pulsosRight = 0; //Iniciar contadores de pulsos a 0
int pulsosPorVolta = 270;  // Pulsos por volta da roda

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

  attachInterrupt(digitalPinToInterrupt(encLeft)  , contarPulsoLeft  , RISING); // Contagem de pulsos do encoder esquerdo
  attachInterrupt(digitalPinToInterrupt(encRight) , contarPulsoRight , RISING);

  
  // Inicializar servo de direção e servo do sensor ultrassom ao centro
  servoDir.attach(pinServoDir);
  servoDir.write(centerAngle); 
  servoUltra.attach(pinServoUltra);
  servoDir.write(90);    

 
}

void loop() {
  if (Serial.available() >= 7) {  // Verifica se há pelo menos 7 bytes disponíveis na Serial
    command = "";
    for (int i = 0; i < 7; i++) {
      command += (char)Serial.read(); // Lê os 7 caracteres do comando
    }

    process_command();
    if (auto_mode == 1) { //Escolha entre modo automático ou manual em função da variável auto_mode recebida por serial
      codeAuto();
    } else {
      codeManual();
    }

    Serial.write(0x06);  // Envia ACK para indicar que o comando foi processado com sucesso

    // Limpa o buffer da Serial para evitar lixo extra
    while (Serial.available() > 0) {
      Serial.read();
    }
  }

}

void read_command(){. // Função para ler o comando da Serial
    if (Serial.available() >= 7){
    command = "";
        for (int i = 0; i < 7; i++) {
            command += (char)Serial.read();
        }
    }
    while (Serial.available()) Serial.read();  // Descarta qualquer lixo restante
}

void process_command(){

  tx1 = command.charAt(0); tx2 = command.charAt(1); tx3 = command.charAt(2);  // Aquisição dos caratéres na posição 0, 1 e 2 da mensagem, usados para direção do carro
  eixo_x = (tx1 - '0') * 100 + (tx2 - '0') * 10 + (tx3 - '0');

  ty1 = command.charAt(3); ty2 = command.charAt(4); ty3 = command.charAt(5);  // Aquição dos caratéres na posição 3, 4 e 5 da mensagem, usados para velocidade do carro
  eixo_y = (ty1 - '0') * 100 + (ty2 - '0') * 10 + (ty3 - '0');

  auto_mode = (command.charAt(6) - '0'); // Aquisição do caratéres na posição 6 da mensagem, usado para modo automático ou manual

  Serial.print(eixo_x); Serial.print(eixo_y); Serial.println(auto_mode);
}

//--------------------------------------------------------------

int converteParaPWM(int valor) { // Converte o valor de velocidade enviado por serial para PWM
  if (valor < 200) {
    return map(valor, 100, 200, 80, 255);
  }
  else if (valor > 200) {
    return map(valor, 200, 300, 255, 80);
  }
}

int convertePosicao(int valor) { // Converte o valor de posição enviado por serial para ângulo do servo
  if (valor < 200) {
    return map(valor, 100, 200, 120, 90);
  }
  else if (valor > 200) {
    return map(valor, 200, 300, 90, 60);
  }
}

int converteAngulo(int valor) { 
  return map (valor, 100, 300, -30, 30);
}

//Codigo do diferencial
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

//Leitura de velocidade dos motores
void readSpeed()  {

  int i1 = 0;          // variável para leitura pulsos motor 1
  int i2 = 0;          // variável para leitura pulsos motor 2

  i1 += pulseIn(8, HIGH, 50000); 
  i2 += pulseIn(4, HIGH, 50000); //500000 significa que se não receber pulso durante 0.5s retorna 0
  

  enc_m1 = (111111 / i1);
  enc_m2 = (111111 / i2); 

  i1 = 0;
  i2 = 0;
}


void codeManual(){
  // Controlo Motores
    if (eixo_y < 200) {
      y = converteParaPWM(eixo_y);
      rot_m1 = LOW; rot_m2 = HIGH;
    }
    else if (eixo_y > 200) {
      y = converteParaPWM(eixo_y);
      rot_m1 = HIGH; rot_m2 = LOW; 
    }
    else   {
      y = 255;

    }

      
    if (eixo_x != 200){
      x = convertePosicao(eixo_x);
    }                       // Controlo Direção
    else{
      x = 90;
    }

    //readSpeed(); // leitura das velocidades de rotação dos motores
    //diff(converteAngulo(eixo_x), y); // chamada da função diff para ajustar velocidades 
  
    // Aplicar movimento
  analogWrite(pwmRight, y);  
  analogWrite(pwmLeft, y);  
  digitalWrite(dirRight, rot_m1);
  digitalWrite(dirLeft, rot_m2);
  servoDir.write(x);
}

//------------------------------------------------------------------------
void moveForward() {
  // Ambas as rodas para frente com mesma velocidade
  speed = 180;
  servoDir.write(centerAngle);        
}

void turnLeft() {
  speed = 200;
  servoDir.write(leftAngle);        
}

void turnRight() {
  speed = 200;
  servoDir.write(rightAngle);        
}

void stopMotors() {
  // Para ambas as rodas
  speed = 255;
  servoDir.write(centerAngle);      
}

void aplicarMovimento() {
  // Centraliza todo o controlo dos motores
  digitalWrite(dirLeft, LOW);
  digitalWrite(dirRight, HIGH);
  analogWrite(pwmLeft, speed);
  analogWrite(pwmRight, speed);
}

//Medição de distância com o sensor ultrassom
float measure() {
  digitalWrite(trigUltra, LOW);  // Limpar o pino trig
	delayMicroseconds(2);  
	digitalWrite(trigUltra, HIGH);  // Enviar pulso de 10us
	delayMicroseconds(10);  
	digitalWrite(trigUltra, LOW);  // Parar o pulso
  duration = pulseIn(echoUltra, HIGH);// Medir duração do pulso de eco
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
    
  }
  
  // Atualizar distâncias globais
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
  // Se alguma das distâncias laterais for menor que a frontal = PAREDE
  if(distLeft <= distFront || distRight <= distFront) {
    isWall = true;   // É um canto pois é detetada a plataforma
  } else {
    isWall = false;  // É um obstáculo isolado
  }
  
  // Retornar servo para posição central
  servoUltra.write(90);
  delay(300);
  
  
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

  
  // 1. Decidir direção
  bool irDireita = (distRight > distLeft); // Se distância à direita do objeto for maior, desviar para a direita
  int anguloDesvio = irDireita ? (rightAngle + 15) : (leftAngle - 15);
  
  

    // Reset rápido dos motores
  digitalWrite(dirLeft, LOW);
  digitalWrite(dirRight, HIGH);
  analogWrite(pwmLeft, 240);
  analogWrite(pwmRight, 240);
  analogWrite(pwmLeft, 255);
  analogWrite(pwmRight, 255);
  
  // 2. Recuar usando encoders
  pulsosLeft = 0;  // Reset do contador
  while(pulsosLeft < pulsosPorVolta/2) {  // Recuar 1/2 de volta
    digitalWrite(dirLeft, HIGH);    
    digitalWrite(dirRight, LOW);
    analogWrite(pwmLeft, normalSpeed);
    analogWrite(pwmRight, normalSpeed);
    contarPulsoLeft();  // Contar pulsos do encoder
    delay(50);  
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
  while(pulsosRight < 0.9 * pulsosPorVolta) {
    digitalWrite(dirLeft, LOW);
    digitalWrite(dirRight, HIGH);
    analogWrite(pwmLeft, normalSpeed);
    analogWrite(pwmRight, normalSpeed);
    contarPulsoRight();  
    delay(100);  
  }
  
  // 6. Virar para o lado oposto para voltar ao caminho
  int anguloRetorno = irDireita ? (leftAngle - 15) : (rightAngle + 15);  // Oposto da direção inicial
  servoDir.write(anguloRetorno);
  delay(300);
  
  // 7. Primeiro ciclo: Avançar com rodas ainda viradas
  pulsosRight = 0;
  while(pulsosRight < 2.3*pulsosPorVolta) {  
    digitalWrite(dirLeft, LOW);
    digitalWrite(dirRight, HIGH);
    analogWrite(pwmLeft, normalSpeed);
    analogWrite(pwmRight, normalSpeed);
    contarPulsoRight();  
    delay(50);
  }
  
  // 8. Centrar rodas para suavizar ângulo de aproximação
  servoDir.write(centerAngle); 
  delay(300);
  
  // 9. Avançar com rodas centradas até encontrar a linha
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
      break;  // Sair do loop quando encontrar a linha
    }
  }
  
  // 9. Viragem final para alinhar com a linha
  servoDir.write(anguloDesvio);  // Virar na mesma direção do desvio inicial
  
  // 10. Avanço curto para ficar centrado na linha
  pulsosRight = 0;
  while(pulsosRight < pulsosPorVolta/1.5) {  // Movimento muito curto para centrar, medido por pulsos do encoder
    digitalWrite(dirLeft, LOW);
    digitalWrite(dirRight, HIGH);
    analogWrite(pwmLeft, normalSpeed);
    analogWrite(pwmRight, normalSpeed);
    contarPulsoRight();  
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
  
  // Desvio concluído
}

void codeAuto() {
  // Verificar obstáculos à frente
  if (checkFront()) {
    
    // Parar e fazer scan
    stopMotors();
    aplicarMovimento();
    delay(300);
    
    // Reset das rodas após paragem, necessário para evitar que o motor fique bloqueado
    digitalWrite(dirLeft, HIGH);    // Pulso para trás
    digitalWrite(dirRight, LOW);
    analogWrite(pwmLeft, 240);
    analogWrite(pwmRight, 240);
    analogWrite(pwmLeft, 255);     // Parar
    analogWrite(pwmRight, 255);
    
    bool ehParede = scan();  // Analisa ambiente e retorna true se for parede
    
    if(ehParede) {
      // É uma PAREDE - PARAR o veículo
      stopMotors();
      aplicarMovimento();
    } else {
      // É um OBSTÁCULO - DESVIAR
      desviarObstaculo();  // Executa desvio
    }
    return;
  }
  
  // Seguir linha normalmente
  int leftSensor = digitalRead(infraLeft);
  int rightSensor = digitalRead(infraRight);
  
  if (leftSensor == HIGH && rightSensor == HIGH) { // Ambos sensores detectam preto
    moveForward();
  } else if (leftSensor == LOW && rightSensor == HIGH) { // Sensor esquerdo vê branco, direito vê preto
    turnRight();
  } else if (leftSensor == HIGH && rightSensor == LOW) {  // Sensor esquerdo vê preto, direito vê branco
    turnLeft();
  } else { // Ambos sensores vêem branco, parar o carro
    stopMotors();
  }
  
  aplicarMovimento();
}
