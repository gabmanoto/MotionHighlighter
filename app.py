from flask import Flask, request, render_template
import sys, os

app = Flask(__name__)

@app.route('/')
def index():
    video_path = request.args.get('video_path')
    method = request.args.get('method')
    os.system("./MotionHighlighter -i="+video_path+" -d=1 -m="+method)
    return render_template('index.html')

if __name__ == "__main__":
    app.run(host="0.0.0.0")
