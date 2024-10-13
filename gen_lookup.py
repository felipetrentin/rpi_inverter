import numpy as np

# Parâmetros
n_points = 341  # Número de pontos (por exemplo, uma volta completa de 0 a 360 graus)
amplitude = 2048  # Amplitude máxima para valores em inteiro de 16 bits (2^15 - 1)

# Gerar os valores do seno
lut = [int(amplitude * np.sin(np.pi*(angle/(n_points-1)))) for angle in range(n_points-1)]

# Imprimir o código C++ com a lookup table
print("#include <stdint.h>\n")
print("const int16_t sine_lut[{}] = {{".format(n_points))
for i, value in enumerate(lut):
    print("{:6d},".format(value), end="")
    if (i + 1) % 8 == 0:  # 8 valores por linha
        print()  # Quebra de linha a cada 8 valores
print(" 0};")
