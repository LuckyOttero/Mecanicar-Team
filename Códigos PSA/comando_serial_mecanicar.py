import pygame   # Biblioteca para capturar entrada do comando xbox
import time     # Módulo para funções relacionadas a tempo time.sleep
import serial   # Biblioteca para comunicação via porta serial

# Configuração porta serial
ser = serial.Serial(port='COM4', baudrate = 112500, timeout = 0.1, write_timeout = 0.1)   # porta serial entre lattepanda e arduino leonardo (integrada/interna)
uno = serial.Serial(port='COM6', baudrate = 112500, timeout = 0.1, write_timeout = 0.5)   # porta serial entre lattepanda e arduino Uno WiFi Rev2 (conexão física via cabo)

# Inicializa Pygame e Joystick
pygame.init()
pygame.joystick.init()

# Função para tentar conectar ao comando Xbox. Deve estar previamente conectado ao pc por Bluetooth
def conectar_comando():
    while True:                    # Loop infinito até conseguir conectar ao comando
        try:
            if pygame.joystick.get_count() > 0:              # Verifica se há pelo menos um joystick (comando) conectado
                joystick = pygame.joystick.Joystick(0)       # Obtém o primeiro joystick (comando) disponível (índice 0)
                joystick.init()                              # Inicializa o joystick para começar a receber eventos
                print(f"Conectado ao comando: {joystick.get_name()}")   # print na consola do sucesso da conexão ao comando
                return joystick                              # Retorna o objeto do joystick para uso posterior no programa
        except Exception as e:
            print(f"Erro ao conectar ao comando: {e}")       # Em caso de erro (por exemplo, não haver conexão prévia do comando) imprime a exceção
        print("Tentando conectar ao comando...")             # print na consola de que ainda não foi estabelecida nenhuma conexão
        time.sleep(2)                                        # pequena espera até tentar de novo

# Conectar ao comando Xbox
joystick = conectar_comando()

# Função para enviar comandos ao Arduino Leonardo
def send_command(command):
	try:
		while ser.out_waiting > 64:       # monitorização do preenchimento do buffer de saída para evitar sobrecarregar o buffer do Arduino
			time.sleep(0.01)              # espera 10ms e tenta novamente
		ser.write(command.encode())       # envio do command codificado em bytes
		ser.flush()                       # aguarda até que todos os dados no buffer de saída sejam enviados
		print(f"Enviado LEO: {command}")  # print do envio na consola para debugging
	except serial.SerialTimeoutException: # caso a escrita no serial falhe
		print(f"Erro LEO Timeout")        # print do erro na consola para debugging

# Função para enviar comandos ao Arduino Uno, similar ao anterior mas com controlo de fluxo para debugging
def send_uno(command):
	try:
		uno.write(command.encode())         # envio do command codificado em bytes
		uno.flush()                         # aguarda até que todos os dados no buffer de saída sejam enviados
        print(f"Enviado UNO: {command}")    # print do envio na consola para debugging

    # Enquanto houver dados disponíveis no buffer de entrada serial (resposta do Uno)
        while uno.in_waiting:
            response = uno.read()               # Lê 1 byte da resposta
            if response == b"\x06":             # Se a resposta for o byte ACK (0x06)
                print("✅ Resposta UNO: ACK")   # print da mensagem de confirmação recebida na consola para debugging
            else:
                print(f"Resposta UNO: {response}")  # print da mensagem recebida na consola para debugging
		

	except serial.SerialTimeoutException:      # caso a escrita no serial falhe
		print(f"Erro UNO Timeout")             # print do erro na consola para debugging

# (no caso do Arduino Uno houveram muitos erros por Timeout daí o código ter sido alterado em relação ao anterior à medida que se foi corrigindo)

# variáveis para captura dos inputs e envio de comandos

servo_base = 1      # 1 parado, 0 ou 2 girar em determinado sentido
servo_ombro = 1
servo_cotovelo = 1
iman = 0            # acionamento do iman
reset_servo = 0     # reset dos servos à posição inicial
auto_mode = 0       # acionar o modo automático

x_was_pressed = False
b_was_pressed = False

while True:
    pygame.event.pump()  # Atualiza eventos do joystick

    # Mapeamento Eixos Analógicos
    eixo_x = round(joystick.get_axis(0), 2)       # Posição eixo x analógico esquerdo arredondado a 2 casas decimais - direção
    eixo_x_Right = round(joystick.get_axis(2), 2) # Posição eixo x analógico direito arredondado a 2 casas decimais - câmara

    #Mapeamento triggers e bumpers
    lt = (round(joystick.get_axis(4), 2) + 1) / 2   # Leitura da posição do trigger lt (valor entre 0 e 1)
    rt = (round(joystick.get_axis(5), 2) + 1) / 2   # Leitura da posição do trigger rt (valor entre 0 e 1)
    lb = joystick.get_button(4)                     # Leitura do estado do botão lb (0 ou 1)
    rb = joystick.get_button(5)                     # Leitura do estado do botão rb (0 ou 1)

    #Mapeamento Botões
    a_pressed = joystick.get_button(0)  # Leitura do estado do botão A (0 ou 1) - limitador de velocidade
    b_pressed = joystick.get_button(1)  # Leitura do estado do botão B (0 ou 1) - acionamento do íman
    x_pressed = joystick.get_button(2)  # Leitura do estado do botão X (0 ou 1) - modo de operação
    y_pressed = joystick.get_button(3)  # Leitura do estado do botão Y (0 ou 1) - reset da posião do manipulador

    # Leitura do D-Pad
    hat_x, hat_y = joystick.get_hat(0) # Leitura do vetor do estado dos botões das setas direcionais (valores de -1, 0 ou 1)

    # Controlo do servo da base usando LB e RB
    if lb:
        servo_base = 2  # Sinal para a esquerda
    elif rb:
        servo_base = 0  # Sinal para a direita
    else:
        servo_base = 1  # Sinal neutro para o servo

    # Controlo do servo do ombro com setas cima/baixo
    if hat_y == 1:
        servo_ombro = 2  # Sinal para a frente
    elif hat_y == -1:
        servo_ombro = 0  # Sinal para trás
    else:
        servo_ombro = 1  # Sinal neutro para o servo do ombro
    
    # Controlo do servo do cotovelo com as setas esquerda/direita
    if hat_x == 1:
        servo_cotovelo = 2  # Sinal para a frente
    elif hat_x == -1:
        servo_cotovelo = 0  # Sinal para a trás
    else:
        servo_cotovelo = 1  # Sinal neutro para o servo do cotovelo

    # Botão Y para reset_servo
    if y_pressed:
        reset_servo = 1   # acionar reset dos servos do manipulador
    else:
        reset_servo = 0

    # Botão B para íman
    if b_pressed and not b_was_pressed:   # Transição de solto para pressionado
        iman = 0 if iman == 1 else 1      # Alterna entre 0 e 1

    b_was_pressed = b_pressed

    if x_pressed and not x_was_pressed:   # Transição de solto para pressionado
        auto_mode = 0 if auto_mode == 1 else 1  # Alterna entre 0 e 1

    x_was_pressed = x_pressed

    # Controlo dos motores
    velocidade_max = 1.0     # Velocidade máxima do motor
    if a_pressed:
        velocidade_max = 0.3  # Reduz a velocidade máxima se o botão A estiver pressionado

    if rt > 0.1:   # pequeno offset caso hajam leituras mínimas quando o trigger não está pressionado
        y = rt * 100 * velocidade_max + 200     # rt gera valores de y entre 200 e 300 (aceleração para a frente)
    elif lt > 0.1:
        y = -lt * 100 * velocidade_max + 200    # lt gera valores de y entre 100 e 200 (aceleração para trás)
    else:
        y = 200                                 # sem input, rodas paradas

    # Controlo da direção das rodas
    if abs(eixo_x) > 0.1:   # pequeno offset caso hajam leituras mínimas quando o analógico não está deslocado
        x = eixo_x * 100 + 200      # valores negativos geram valor de x de 100 a 200 enquanto que positivos de 200 a 300
    else:
        x = 200                     # sem input, rodas centradas

    # Controlo da direção da câmara
    if abs(eixo_x_Right) > 0.1:     # pequeno offset caso hajam leituras mínimas quando o analógico não está deslocado
        x_Right = eixo_x_Right * 100 + 200   # valores negativos geram valor de x de 100 a 200 enquanto que positivos de 200 a 300
    else:
        x_Right = 200                        # sem input, camâra centrada

    x = int(x)               # conversão para inteiro (envio de apenas 3 dígitos, sem casa decimais)
    y = int(y)               # conversão para inteiro (envio de apenas 3 dígitos, sem casa decimais)
    x_Right = int(x_Right)   # conversão para inteiro (envio de apenas 3 dígitos, sem casa decimais)

    motor_command = f"{servo_base}{servo_ombro}{servo_cotovelo}{reset_servo}{iman}{x_Right}"  # string dos inputs para o Arduino Leonardo
    uno_command = f"{x}{y}{auto_mode}"                                                        # string dos inputs para o Arduino Uno WiFi Rev2

    send_command(motor_command)  # Envio do comando para o Arduino Leonardo
    send_uno(uno_command)        # Envio do comando para o Arduino Uno WiFi Rev2

    time.sleep(0.1)  # Delay que determina a cadência do envio dos inputs