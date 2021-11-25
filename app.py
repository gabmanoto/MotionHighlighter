# from flask import Flask, request, render_template
# import sys, os

# app = Flask(__name__)

# @app.route('/')
# def index():
#     video_path = request.args.get('video_path')
#     method = request.args.get('method')
#     os.system("./MotionHighlighter -i="+video_path+" -d=1 -m="+method)
#     return render_template('index.html')

# if __name__ == "__main__":
#     app.run(host="0.0.0.0")

from flask import Flask

#this is where you will be uploading the file to
INPUT_FOLDER = "input/"
OUTPUT_FOLDER = "static/output/images"
app = Flask(__name__)
app.secret_key = "secret key"
app.config['INPUT_FOLDER'] = INPUT_FOLDER
app.config['OUTPUT_FOLDER'] = OUTPUT_FOLDER
# app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024