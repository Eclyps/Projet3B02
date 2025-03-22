import requests
import random
import time

# Adresse de base du serveur
base_url = "http://10.221.29.254:5000/post"

# Durée de la simulation (en secondes)
duration_seconds = 60

# Fréquence d'envoi (en secondes)
time_step = 0.5

# Alerte spécifique à envoyer après 15 secondes (une seule fois)
alerte_sent = False
alerte_time_trigger = 15  # secondes

# Initialisation des joueurs avec jambes droite/gauche
players = {
    1: {  # Joueur 1
        "ids": {"droite": 1, "gauche": 2},
        "base": {"distance": 3000.0, "distanceSprint": 100.0, "vitesse": 3.0, "vitesseMax": 3.0},
        "droite": {"distance": 3000.0, "distanceSprint": 100.0},
        "gauche": {"distance": 3000.0, "distanceSprint": 100.0}
    },
    2: {  # Joueur 2
        "ids": {"droite": 3, "gauche": 4},
        "base": {"distance": 3000.0, "distanceSprint": 100.0, "vitesse": 3.0, "vitesseMax": 3.0},
        "droite": {"distance": 3000.0, "distanceSprint": 100.0},
        "gauche": {"distance": 3000.0, "distanceSprint": 100.0}
    }
}

# Boucle d'envoi pendant la durée définie
start_time = time.time()

while time.time() - start_time < duration_seconds:
    elapsed_time = time.time() - start_time

    # Envoi de l'alerte à 15 secondes si pas encore envoyé
    if not alerte_sent and elapsed_time >= alerte_time_trigger:
        alerte_id = 1  # ID de la jambe droite du joueur 1
        url_alerte = f"{base_url}/alerte/{alerte_id}"
        try:
            response = requests.post(url_alerte)
            print(f"ALERTE envoyée à {url_alerte} -> {response.status_code} {response.reason}")
            alerte_sent = True  # On ne l'envoie qu'une seule fois
        except requests.exceptions.RequestException as e:
            print(f"Erreur lors de l'envoi de l'alerte à {url_alerte} : {e}")

    for player_id, data in players.items():
        
        base = data["base"]
        
        # Générer une nouvelle vitesse avec une variation
        delta_vitesse = random.uniform(-1.0, 1.0)
        nouvelle_vitesse = max(0, base["vitesse"] + delta_vitesse)

        # Calcul de distance supplémentaire
        distance_ajout = nouvelle_vitesse * time_step

        # Mise à jour des distances
        base["distance"] += distance_ajout

        # Sprint uniquement si vitesse > 25 km/h -> 6.94 m/s
        if nouvelle_vitesse >= 6.94:
            base["distanceSprint"] += distance_ajout
            sprinting = True
        else:
            sprinting = False

        # Mise à jour de la vitesseMax si nécessaire
        if nouvelle_vitesse > base["vitesseMax"]:
            base["vitesseMax"] = nouvelle_vitesse

        # Mettre à jour la vitesse actuelle
        base["vitesse"] = nouvelle_vitesse

        # Boucle sur chaque jambe (droite et gauche)
        for jambe in ["droite", "gauche"]:
            leg = data[jambe]
            entity_id = data["ids"][jambe]

            # Distance jambe avec variation
            leg["distance"] = base["distance"] + random.uniform(-1.0, 1.0)

            # Gestion du sprint pour la jambe si en sprint
            if sprinting:
                leg["distanceSprint"] += distance_ajout + random.uniform(-0.5, 0.5)

            # Empêcher distanceSprint > distance
            leg["distanceSprint"] = min(leg["distanceSprint"], leg["distance"])

            # Arrondir les données avant envoi
            distance = round(leg["distance"], 1)
            distance_sprint = round(leg["distanceSprint"], 1)
            vitesse = round(base["vitesse"], 1)
            vitesse_max = round(base["vitesseMax"], 1)

            # Préparer les données à envoyer
            envois = {
                "distance": distance,
                "distanceSprint": distance_sprint,
                "vitesse": vitesse,
                "vitesseMax": vitesse_max
            }

            # Envoi des données à chaque endpoint
            for endpoint, valeur in envois.items():
                url = f"{base_url}/{endpoint}/{entity_id}/{valeur}"
                try:
                    response = requests.post(url)
                    print(f"POST {url} -> {response.status_code} {response.reason}")
                except requests.exceptions.RequestException as e:
                    print(f"Erreur lors de l'envoi à {url} : {e}")

    # Pause avant la prochaine itération
    time.sleep(time_step)

print("Fin de la simulation d'envoi de données.")
