from flask import Flask, render_template, send_from_directory, jsonify, request
import pandas as pd
import matplotlib.pyplot as plt
from fpdf import FPDF
import os
import csv
from datetime import datetime

app = Flask(__name__)

# Variables mémoire
liste_alerte = []
dic_vitesse = {}
dic_distance = {}
dic_vitesse_max = {}
dic_distance_sprint = {}
message = ""
competition = ""
adversaire = ""
domicile = False
type_terrain = ""

# Dossiers de travail
DATA_DIR = "data"
PDF_DIR = "../pdf_reports"

# Colonnes CSV
COLUMNS = ["Timestamp", "Vitesse", "Distance", "Vitessemax", "Distancesprint", "Alerte"]

# Initialisation dossier data
os.makedirs(DATA_DIR, exist_ok=True)


# Création fichier CSV si absent
def init_csv_file(post_id):
    file_path = os.path.join(DATA_DIR, f"ID_{post_id}.csv")
    if not os.path.exists(file_path):
        with open(file_path, mode='w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(COLUMNS)


# MAJ csv
def write_event(post_id, data_type, value):
    init_csv_file(post_id)

    file_path = os.path.join(DATA_DIR, f"ID_{post_id}.csv")

    # Charger données existantes
    df = pd.read_csv(file_path)

    # Vérifie si on remplit une ligne ou en ajoute une
    if df.empty or not df.iloc[-1].isnull().any():
        # Ajouter une ligne
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        new_row = {
            "Timestamp": timestamp,
            "Vitesse": "",
            "Distance": "",
            "Vitessemax": "",
            "Distancesprint": "",
            "Alerte": "-"
        }
        df = df.append(new_row, ignore_index=True)

    # Met à jour la dernière ligne avec la valeur reçue
    col_name = next((col for col in COLUMNS if col.lower() == data_type.lower()), None)
    if col_name is None:
        print(f"Type {data_type} non trouvé !")
        return

    df.at[df.index[-1], col_name] = value

    # Sauvegarde du fichier
    df.to_csv(file_path, index=False)


# Lecture des données CSV
def read_data_from_csv(post_id):
    file_path = os.path.join(DATA_DIR, f"ID_{post_id}.csv")
    if not os.path.exists(file_path):
        print(f"Fichier {file_path} non trouvé !")
        return None

    df = pd.read_csv(file_path)

    # Convertir les timestamps si présents
    if 'Timestamp' in df.columns:
        df['Timestamp'] = pd.to_datetime(df['Timestamp'])

    return df


# Graphique : courbe de vitesse
def plot_speed(df, post_id):
    plt.figure(figsize=(8, 4))
    plt.plot(df['Timestamp'], df['Vitesse'].astype(float), marker='o', color='blue')
    plt.title(f'Vitesse en fonction du temps (ID {post_id})')
    plt.xlabel('Temps')
    plt.ylabel('Vitesse (km/h)')
    plt.xticks(rotation=45)
    plt.tight_layout()
    graph_path = f"speed_time_{post_id}.png"
    plt.savefig(graph_path)
    plt.close()
    return graph_path


# Graphique : proportion sprint / normal
def plot_sprint_vs_normal(df, post_id):
    sprint_distance = df['Distancesprint'].iloc[-1] if not df['Distancesprint'].empty and df['Distancesprint'].iloc[
        -1] != '-' else 0
    normal_distance = df['Distance'].iloc[-1] if not df['Distance'].empty and df['Distance'].iloc[-1] != '-' else 0

    if sprint_distance + normal_distance == 0:
        print(f"Aucune donnée de distance pour ID {post_id}")
        return None

    plt.figure(figsize=(5, 5))
    labels = ['Sprint', 'Normal']
    values = [float(sprint_distance), float(normal_distance) * 1000]
    colors = ['red', 'green']
    plt.pie(values, labels=labels, autopct='%1.1f%%', colors=colors)
    plt.title(f"Sprint vs Normal (ID {post_id})")

    pie_path = f"sprint_normal_{post_id}.png"
    plt.savefig(pie_path)
    plt.close()

    return pie_path


# Génération PDF d'un rapport individuel
def generate_pdf_report(post_id, output_path=None):
    df = read_data_from_csv(post_id)
    if df is None:
        return

    speed_plot = plot_speed(df, post_id)
    sprint_pie = plot_sprint_vs_normal(df, post_id)

    vmax = df['Vitessemax'].astype(float).max() if 'Vitessemax' in df.columns else 0
    distance = df['Distance'].iloc[-1] if not df['Distance'].empty else 0
    sprint_distance = df['Distancesprint'].iloc[-1] if not df['Distancesprint'].empty else 0
    nb_alertes = df['Alerte'].fillna('-').apply(lambda x: x != '-').sum() if 'Alerte' in df.columns else 0

    pdf = FPDF()
    pdf.add_page()

    pdf.set_font("Arial", size=16)
    pdf.cell(200, 10, f"Rapport Joueur ID {post_id}", ln=True, align='C')

    pdf.set_font("Arial", size=12)
    pdf.ln(10)
    pdf.cell(0, 10, f"Compétition : {competition}", ln=True)
    pdf.cell(0, 10, f"Adversaire : {adversaire}", ln=True)
    pdf.cell(0, 10, f"Domicile : {'Oui' if domicile else 'Non'}", ln=True)
    pdf.cell(0, 10, f"Type de terrain : {type_terrain}", ln=True)

    pdf.ln(10)
    pdf.cell(0, 10, f"Vitesse Max : {vmax} km/h", ln=True)
    pdf.cell(0, 10, f"Distance Totale : {distance} km", ln=True)
    pdf.cell(0, 10, f"Distance en Sprint : {sprint_distance} m", ln=True)
    pdf.cell(0, 10, f"Nombre d'alertes : {nb_alertes}", ln=True)

    pdf.ln(10)
    pdf.cell(0, 10, "Courbe de vitesse :", ln=True)
    if speed_plot:
        pdf.image(speed_plot, w=180)

    pdf.ln(10)
    pdf.cell(0, 10, "Proportion Sprint vs Normal :", ln=True)
    if sprint_pie:
        pdf.image(sprint_pie, w=120)

    if output_path is None:
        output_path = f"rapport_ID_{post_id}.pdf"

    pdf.output(output_path)
    print(f"✅ PDF généré : {output_path}")


# Génération de tous les rapports PDF
def generate_all_reports():
    os.makedirs(PDF_DIR, exist_ok=True)

    for filename in os.listdir(DATA_DIR):
        if filename.endswith(".csv") and filename.startswith("ID_"):
            post_id = filename.replace("ID_", "").replace(".csv", "")
            print(f"➡️ Génération du PDF pour l'ID {post_id}")
            pdf_filepath = os.path.join(PDF_DIR, f"rapport_ID_{post_id}.pdf")
            generate_pdf_report(post_id, pdf_filepath)

    print(f"✅ Tous les rapports PDF sont générés dans {PDF_DIR}")


# === SERVER HTTP ===

@app.route("/")
def hello_world():
    return "<p> Hello World!</p>"


@app.route('/postMessage/<int:postId>/<string:messageArduino>', methods=["POST"])
def post_message(postId, messageArduino):
    global message
    message = messageArduino
    return f'Message reçu : {message}'


@app.route('/postInfoCompetition', methods=["POST"])
def post_info_competition():
    global competition, adversaire, domicile, type_terrain
    data = request.get_json()
    competition = data.get('competition', '')
    adversaire = data.get('adversaire', '')
    domicile = data.get('domicile', False)
    type_terrain = data.get('type_terrain', '')
    return jsonify({
        "message": "Infos compétition reçues",
        "competition": competition,
        "adversaire": adversaire,
        "domicile": domicile,
        "type_terrain": type_terrain
    })

@app.route('/post/alerte/<int:post_id>', methods=["POST"])
def post_alerte(post_id):
    liste_alerte.append(post_id)
    write_event(post_id, "Alerte", "Alerte")
    return f'Alerte {post_id}'


@app.route('/post/vitesse/<int:post_id>/<float:vitesse>', methods=["POST"])
def post_vitesse(post_id, vitesse):
    vit = round(vitesse * 3.6, 1)  # m/s -> km/h
    dic_vitesse[post_id] = vit
    write_event(post_id, "Vitesse", vit)
    return f'Vitesse enregistrée {post_id} -> {vit}'


@app.route('/post/distance/<int:post_id>/<float:distance>', methods=["POST"])
def post_distance(post_id, distance):
    dist_km = round(distance / 1000, 1)
    dic_distance[post_id] = dist_km
    write_event(post_id, "Distance", dist_km)
    return f'Distance enregistrée {post_id} -> {dist_km} km'


@app.route('/post/vitesseMax/<int:post_id>/<float:vitesse>', methods=["POST"])
def post_vitesse_max(post_id, vitesse):
    vit = round(vitesse * 3.6, 1)  # m/s -> km/h
    dic_vitesse_max[post_id] = vit
    write_event(post_id, "VitesseMax", vit)
    return f'VitesseMax enregistrée {post_id} -> {vit}'


@app.route('/post/distanceSprint/<int:post_id>/<float:distance>', methods=["POST"])
def post_distance_sprint(post_id, distance):
    dic_distance_sprint[post_id] = int(distance)
    write_event(post_id, "Distancesprint", int(distance))
    return f'DistanceSprint enregistrée {post_id} -> {distance} m'


@app.route('/get/message')
def get_message():
    return jsonify(message)


@app.route('/get/alerte')
def get_alerte():
    global liste_alerte
    alerte_a_envoyer = liste_alerte.copy()
    liste_alerte.clear()
    return jsonify(alerte_a_envoyer)


@app.route('/get/vitesse')
def get_vitesse():
    return jsonify(dic_vitesse)


@app.route('/get/distance')
def get_distance():
    return jsonify(dic_distance)


@app.route('/get/vitesseMax')
def get_vitesse_max():
    return jsonify(dic_vitesse_max)


@app.route('/get/distanceSprint')
def get_distance_sprint():
    return jsonify(dic_distance_sprint)


@app.route('/download/reports')
def download_reports():
    generate_all_reports()
    pdf_files = [f for f in os.listdir(PDF_DIR) if f.endswith(".pdf")]
    return render_template('download_page.html', pdf_files=pdf_files)


@app.route('/download/<filename>')
def download_file(filename):
    return send_from_directory(PDF_DIR, filename, as_attachment=True)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
