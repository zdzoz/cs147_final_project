from flask import Flask, request, jsonify, render_template

app = Flask(__name__)

door_open_count = [0]


def curr_count():
    return door_open_count[-1]


@app.route("/", methods=["GET", "POST"])
def door():
    global door_open_count
    if request.method == "POST":
        door_open_count.append(curr_count() + 1)
        return jsonify({"message": "Door opened", "door_open_count": curr_count()}), 200
    elif request.method == "GET":
        return jsonify({"door_open_count": curr_count()}), 200


@app.route("/reset", methods=["POST"])
def reset():
    global door_open_count
    if request.method == "POST":
        door_open_count.append(0)
    else:
        return jsonify({"message": "error"}), 404
    return jsonify({"message": "Count reset", "door_open_count": curr_count()}), 200


@app.route("/dashboard", methods=["GET"])
def dashboard():
    global door_open_count
    return render_template(template_name_or_list="dashboard.html", data=door_open_count)


if __name__ == "__main__":
    app.run(debug=True)
