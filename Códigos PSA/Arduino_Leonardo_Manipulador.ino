#include <Arduino.h>    
#include <Servo.h>

//--------------------------------------------------- VARIÁVEIS ------------------------------------------------------------------------
 
int servo_base;       // variável do input para servo da base
int servo_ombro;      // variável do input para servo do ombro
int servo_cotovelo;   // variável do input para servo do cotovelo
int servo_camara;     // variável do input para servo da câmara
int servo_reset = 0;  // variável do reset da posição dos servos do manipulador
int iman_on = 0;      // variável para acionamento do íman

char base, ombro, cotovelo, reset, iman, c1, c2, c3;    // variáveis leitura dos dados recebidos por serial command

float h = 0.115;      // distância das rodas ao centro em mm
float l = 0.32;       // distância entre rodas dianteiras e traseiras em mm

Servo servoBase;      // servo do base
Servo servoOmbro;     // servo do ombro
Servo servoCotovelo;  // servo do cotovelo
Servo servoCamara;    // servo da câmara

const int relePin = 2;// pino de acionamento do relé do íman

// variáveis para escrever a posição dos servomotores do manipulador (em microssegundos)
int currentBasePulse = 1500;   // posição inicial neutra para servo_base
int currentOmbroPulse = 1500;  // posição inicial neutra para servo_ombro
int currentCotoveloPulse = 1500;  // posição inicial neutra para servo_cotovelo

// Definição de incrementos e de limites para segurança
int stepBase = 7;           // incremento da posição em microssegundos     
int maxBasePulse = 2500;    // posição máxima do servo da base
int minBasePulse = 500;     // posição mínima do servo da base

int stepOmbro = 4;     
int maxOmbroPulse = 2000;
int minOmbroPulse = 1000; 

int stepCotovelo = 4;     
int maxCotoveloPulse = 2500;
int minCotoveloPulse = 500;

// Para a câmara foram definidos os limites e a posição em ângulo
int angleCamara = 90;

String command;   // string para leitura dos dados recebidos por serial

void setup() {
    Serial.begin(115200);       // iniciação da comunicação serial com baudrate 115 200
    pinMode(relePin, OUTPUT);   // pin do relé definido como output
    servoBase.attach(11);       // atribuição do pino 11 para controlo do servo da base
    servoOmbro.attach(13);      // atribuição do pino 13 para controlo do servo do ombro
    servoCotovelo.attach(6);    // atribuição do pino 6 para controlo do servo do cotovelo
    servoCamara.attach(5);      // atribuição do pino 5 para controlo do servo da câmara

    servoBase.writeMicroseconds(1500);      // servos iniciam na posição incial
    servoOmbro.writeMicroseconds(1500);
    servoCotovelo.writeMicroseconds(1500);
    servoCamara.write(angleCamara);
    delay(20);
}

void loop() {
    read_command();       // leitura da mensagem recebida por serial

    process_command();    // atualização das variáveis de acordo com a mensagem recebida

    if (servo_reset == 1) {       // caso o reset esteja ativo há prioridade de ativação em relação aos outros controlos
        currentBasePulse = 1500;  // reset da posição dos servos do manipulador para a posição inicial
        currentOmbroPulse = 1500;
        currentCotoveloPulse = 1500;
    } 
    else {                       // caso o reset esteja desativado, pode ser feito o controlo do manipulador servo a servo

        if (servo_base > 1) {                 // se o comando recebido > 1 é aumentada a posição de forma incremental do servo da base, tendo em conta o limite máximo
            currentBasePulse += stepBase;
            if (currentBasePulse > maxBasePulse) {
                currentBasePulse = maxBasePulse;
            }
        } 
        else if (servo_base < 1) {           // se o comando recebido < 1 é diminuída a posição de forma incremental do servo da base, tendo em conta o limite mínimo
            currentBasePulse -= stepBase;
            if (currentBasePulse < minBasePulse) {
                currentBasePulse = minBasePulse;
                }
        }

        // mesma estrutura que acima, mas controlando o servo do ombro
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

        // mesma estrutura que acima, mas controlando o servo do cotovelo
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

        // alteração da variável relativa à posição do servo da câmara, de acordo com a conversão definida entre movimento do joystick e rotação do servo 
        angleCamara = converteAngulo(servo_camara);         

        if (iman_on == 1) {              // acionamento do relé caso a variável esteja a 1 => conduz 12V pelo íman ligando-o
          digitalWrite(relePin, HIGH);
        }
        else if (iman_on == 0) {        // relé não acionado caso a variável esteja a 0 => não passa corrente pelo íman => desligado
          digitalWrite(relePin, LOW);
        }
    }
           
    // escrita das variáveis atualizados com o código anterior, para os respetivos servos
    servoBase.writeMicroseconds(currentBasePulse);
    servoOmbro.writeMicroseconds(currentOmbroPulse);
    servoCotovelo.writeMicroseconds(currentCotoveloPulse);
    servoCamara.write(angleCamara);

    delay(20);
}

void read_command(){
    if (Serial.available() >= 8){              // verifica se a mensagem recebida por serial tem um mínimo de 8 caracteres
    command = "";                              // iguala a variavél do comando recebido a uma string vazia para escrever o novo comando
        for (int i = 0; i < 8; i++) {          // ciclo for que realiza 8 leituras e adiciona os valores lidos à string command
            command += (char)Serial.read();
        }
    }
    while (Serial.available()) Serial.read();  // Limpeza do buffer após leitura
}

void process_command(){
    // seleção dos carateres individuais e atribuição às variáveis criadas inicialmente para o efeito
    base = command.charAt(0); ombro = command.charAt(1); cotovelo = command.charAt(2); reset = command.charAt(3); iman = command.charAt(4);

    // tratamento das strings recebidas para representarem os números inteiros pretendidos (segundo a tabela ASCII)
    servo_base    = (base - '0');       // recebe um valor de 0, 1 ou 2
    servo_ombro   = (ombro - '0');      // recebe um valor de 0, 1 ou 2
    servo_cotovelo = (cotovelo - '0');  // recebe um valor de 0, 1 ou 2
    servo_reset   = (reset - '0');      // recebe um valor de 0 ou 1
    iman_on       = (iman - '0');       // recebe um valor de 0 ou 1

    // leitura dos 3 carateres relativos à posição do servo da câmara e respetivo tratamento para variável inteira de 3 digítos
    c1 = command.charAt(5); c2 = command.charAt(6); c3 = command.charAt(7);
    servo_camara = (c1 - '0') * 100 + (c2 - '0') * 10 + (c3 - '0');   // Valor de 100 a 300
}


int converteAngulo(int valor) {    // função que converte os valores recebidos de 100 a 300 em posições angulares do servo da câmara
  return map (valor, 100, 300, 50, 130);
}
