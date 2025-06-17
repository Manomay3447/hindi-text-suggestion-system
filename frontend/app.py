from flask import Flask, request, jsonify, send_from_directory
from flask_cors import CORS
import os
import time

app = Flask(__name__, static_folder="/var/www/hindi_suggestions", static_url_path="")
CORS(app)

@app.route("/")
def serve_index():
    # Serve index.html from the static folder
    return send_from_directory(app.static_folder, "index.html")

@app.route("/suggest", methods=["POST"])
def suggest():
    data = request.get_json()
    user_input = data.get("text", "")

    if not os.path.exists("/var/www/hindi_suggestions/fifos/c_input_fifo") or not os.path.exists("/var/www/hindi_suggestions/fifos/c_output_fifo"):
        return jsonify({"error": "FIFO pipes not found. Please ensure the C server is running."}), 500


    try:
        # Send input to C program
        with open("/var/www/hindi_suggestions/fifos/c_input_fifo", "w", encoding="utf-8") as fifo_in:
            fifo_in.write(user_input + "\n")
            fifo_in.flush()

        # Read output from C program
        with open("/var/www/hindi_suggestions/fifos/c_output_fifo", "r", encoding="utf-8") as fifo_out:
            lines = fifo_out.readlines()
            # Clean and parse output
            suggestions = [line.strip() for line in lines if line.strip() and not line.startswith("Suggestions for:")]
            return jsonify(suggestions[:10])

    except Exception as e:
        return jsonify({"error": str(e)}), 500


if __name__ == "__main__":
    app.run(debug=True)
