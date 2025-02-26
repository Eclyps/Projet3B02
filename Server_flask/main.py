from flask import Flask, jsonify

app = Flask(__name__)

liste_alerte = []
dic_vitesse = {}
dic_distance = {}

@app.route("/")
def hello_world():
    return "<p> World!</p>"

@app.route('/post/alerte/<int:post_id>', methods=["POST"])
def post_alerte(post_id):
    # show the post with the given id, the id is an integer
    liste_alerte.append(post_id)
    return f'Post {post_id}'
#curl.exe -X POST http://127.0.0.1:5000/post/alerte/123 pour tester

@app.route('/post/vitesse/<int:post_id>/<float:vitesse>', methods=["POST"])
def post_vitesse(post_id, vitesse):
    # show the post with the given id, the id is an integer
    dic_vitesse[post_id] = vitesse
    return f'Post {post_id}'

@app.route('/post/distance/<int:post_id>/<float:distance>',methods=["POST"])
def post_distance(post_id, distance):
    # show the post with the given id, the id is an integer
    dic_distance[post_id] = distance
    return f'Post {post_id}'


@app.route('/get/alerte')
def get_alerte():
    return jsonify(liste_alerte)

@app.route('/get/vitesse')
def get_vitesse():
    return jsonify(dic_vitesse)

@app.route('/get/distance')
def get_distance():
    return jsonify(dic_distance)


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
