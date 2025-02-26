import numpy as np
import matplotlib.pyplot as plt

current = np.arange(4, 5.1, 0.1)  # De 4A à 5A avec un pas de 0.1A

temperature = 40 + (current - 4) * 10  # 

# Tracer la courbe
plt.plot(current, temperature, marker='o', linestyle='-', color='b')
plt.title('Courbe de Température en fonction du Courant')
plt.xlabel('Courant (A)')
plt.ylabel('Température (°C)')
plt.grid(True)
plt.show()
