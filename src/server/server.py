from flask import Flask, request, jsonify, render_template
import datetime

app = Flask(__name__)


def get_time():
    x = datetime.datetime.now()
    return x.strftime("%m/%d (%H:%M:%S)")


door_open_count = [(0, get_time())]


def curr_count():
    return door_open_count[-1][0]


@app.route("/", methods=["GET", "POST"])
def door():
    global door_open_count
    if request.method == "POST":
        door_open_count.append((curr_count() + 1, get_time()))
        return jsonify({"message": "Door opened", "door_open_count": curr_count()}), 200
    elif request.method == "GET":
        return jsonify({"door_open_count": curr_count()}), 200


@app.route("/reset", methods=["POST"])
def reset():
    global door_open_count
    if request.method == "POST":
        door_open_count.append((0, get_time()))
    else:
        return jsonify({"message": "error"}), 404
    return jsonify({"message": "Count reset", "door_open_count": curr_count()}), 200


@app.route("/dashboard", methods=["GET"])
def dashboard():
    global door_open_count
    data = list(map(lambda x: x[0], door_open_count))
    times = list(map(lambda x: x[1], door_open_count))
    return render_template(
        template_name_or_list="dashboard.html", times=times, data=data
    )


if __name__ == "__main__":
    app.run(debug=True)
