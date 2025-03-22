import requests
import random
import time

# Adresse de base du serveur
base_url = "http://10.221.29.254:5000/post"

# Fonction pour générer les valeurs aléatoires en fonction de l'endpoint
def generate_random_value(endpoint):
    if endpoint == "distanceSprint":
        return round(random.uniform(100, 200), 1)  # Exemple : 100.0 à 200.0
    elif endpoint == "distance":
        return round(random.uniform(3000, 4000), 1)  # Exemple : 3000.0 à 4000.0
    elif endpoint == "vitesse":
        return round(random.uniform(0, 8), 1)  # Exemple : 10.0 à 15.0
    elif endpoint == "vitesseMax":
        return round(random.uniform(5, 8), 1)  # Exemple : 25.0 à 40.0
    else:
        return 0

# Liste des endpoints et IDs
requests_data = [
    ("distanceSprint", 1),
    ("distanceSprint", 2),
    ("distance", 1),
    ("distance", 2),
    ("vitesse", 1),
    ("vitesse", 2),
    ("vitesseMax", 1),
    ("vitesseMax", 2),
]

# Envoi des requêtes POST avec des valeurs aléatoires
for endpoint, id_val in requests_data:
    random_value = generate_random_value(endpoint)
    url = f"{base_url}/{endpoint}/{id_val}/{random_value}"
    try:
        response = requests.post(url)
        print(f"POST {url} -> {response.status_code} {response.reason}")
    except requests.exceptions.RequestException as e:
        print(f"Erreur lors de l'envoi à {url} : {e}")
    
    time.sleep(0.5)  # Petite pause pour ne pas surcharger le serveur (facultatif)
