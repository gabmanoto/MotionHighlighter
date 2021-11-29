from flask import Flask

#this is where you will be uploading the file to
INPUT_FOLDER = "input/"
OUTPUT_FOLDER = "static/output/images"
app = Flask(__name__)
app.secret_key = "secret key"
app.config['INPUT_FOLDER'] = INPUT_FOLDER
app.config['OUTPUT_FOLDER'] = OUTPUT_FOLDER
# app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024