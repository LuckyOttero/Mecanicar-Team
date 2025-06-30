import pygame
import time
import serial

# Configuração porta serial
ser = serial.Serial(port='COM4', baudrate = 112500, timeout = 0.1, write_timeout = 0.1)
uno = serial.Serial(port='COM6', baudrate = 112500, timeout = 0.1, write_timeout = 0.5)

# Inicializa Pygame e Joystick
pygame.init()
pygame.joystick.init()

# Função para tentar conectar ao comando Xbox
def conectar_comando():
    while True:
        try:
            if pygame.joystick.get_count() > 0:
                joystick = pygame.joystick.Joystick(0)
                joystick.init()
                print(f"✅ Conectado ao comando: {joystick.get_name()}")
                return joystick
        except Exception as e:
            print(f"Erro ao conectar ao comando: {e}")
        print("🔄 Tentando conectar ao comando...")
        time.sleep(2)

# Conectar ao comando Xbox
joystick = conectar_comando()

# Função para enviar comandos ao Arduino
def send_command(command):
	try:
		while ser.out_waiting > 64:
			time.sleep(0.01)
		ser.write(command.encode())
		ser.flush()
		print(f"Enviado LEO: {command}")
	except serial.SerialTimeoutException:
		print(f"Erro LEO Timeout")
	
    
        
# Função para enviar comandos ao Arduino
def send_uno(command):
	try:
		while uno.out_waiting > 8:
			time.sleep(0.01)
		uno.write(command.encode())
		uno.flush()
		
		start_ack = time.time()
		while time.time() - start_ack < 0.1:
			if uno.in_waiting:
				resp = uno.read(1)
				if resp == b'\x06' or resp == b'\n':
					break
					
		print(f"Enviado UNO: {command}")
	except serial.SerialTimeoutException:
		print(f"Erro UNO Timeout")

# Loop principal
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
    eixo_x = round(joystick.get_axis(0), 2)  # Posição eixo x analógico esquerdo
    # eixo_y = round(joystick.get_axis(1), 2)  # Posição eixo y analógico esquerdo
    eixo_x_Right = round(joystick.get_axis(2), 2)  # controlo servo camâra

    #Mapeamento triggers e bumpers
    lt = (round(joystick.get_axis(4), 2) + 1) / 2   # Left Trigger (LT)
    rt = (round(joystick.get_axis(5), 2) + 1) / 2   # Right Trigger (RT)
    lb = joystick.get_button(4)  # Botão LB (9)
    rb = joystick.get_button(5)  # Botão RB (10)

    #Mapeamento Botões
    a_pressed = joystick.get_button(0)  # Botão A (0)
    b_pressed = joystick.get_button(1)  # Botão B (1)
    x_pressed = joystick.get_button(2)  # Botão X (2)
    y_pressed = joystick.get_button(3)  # Botão Y (3)

    # Leitura do D-Pad
    hat_x, hat_y = joystick.get_hat(0)

    # Controle do servo principal usando LB e RB
    if lb:  # Sinal para cima
        servo_base = 2
    elif rb:  # Sinal para baixo
        servo_base = 0
    else:
        servo_base = 1  # Sinal neutro para o servo

    # Controle da garra com setas cima/baixo
    if hat_y == 1:
        servo_ombro = 2
    elif hat_y == -1:
        servo_ombro = 0
    else:
        servo_ombro = 1  # Sinal neutro para o servo da garra
    
    # Controle do servo extra com as setas esquerda/direita
    if hat_x == 1:
        servo_cotovelo = 2
    elif hat_x == -1:
        servo_cotovelo = 0
    else:
        servo_cotovelo = 1  # Sinal neutro para o servo extra

    # Botão Y para reset_servo
    if y_pressed:
        reset_servo = 1
    else:
        reset_servo = 0

    # Botão B para iman
    if b_pressed and not b_was_pressed:
        # Transição de solto para pressionado
        iman = 0 if iman == 1 else 1  # Alterna entre 0 e 1

    b_was_pressed = b_pressed

    if x_pressed and not x_was_pressed:
        # Transição de solto para pressionado
        auto_mode = 0 if auto_mode == 1 else 1  # Alterna entre 0 e 1

    x_was_pressed = x_pressed

    # Controle do carro (motores)
    velocidade_max = 1.0  # Velocidade máxima do motor
    if a_pressed:
        velocidade_max = 0.3  # Reduz a velocidade máxima se o botão A estiver pressionado

    if rt > 0.1:
        y = rt * 100 * velocidade_max + 200
    elif lt > 0.1:
        y = -lt * 100 * velocidade_max + 200
    else:
        y = 200

    if abs(eixo_x) > 0.1:
        x = eixo_x * 100 + 200
    else:
        x = 200

    if abs(eixo_x_Right) > 0.1:
        x_Right = eixo_x_Right * 100 + 200
    else:
        x_Right = 200

    x = int(x)
    y = int(y)
    x_Right = int(x_Right)

    motor_command = f"{x}{y}{auto_mode}{servo_base}{servo_ombro}{servo_cotovelo}{reset_servo}{iman}{x_Right}"  
    uno_command = f"{x}{y}{auto_mode}"

    # print(motor_command)
    send_command(motor_command)  # Envio do comando para o Arduino
    send_uno(uno_command)

    time.sleep(0.1)  # Pequeno delay para evitar sobrecarga
