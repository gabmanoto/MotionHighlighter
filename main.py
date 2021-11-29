import os
import urllib.request
import json
from app import app
from flask import Flask, flash, request, redirect, render_template, url_for, send_from_directory, jsonify
from werkzeug.utils import secure_filename
from glob import glob
from io import BytesIO
from zipfile import ZipFile
from flask import send_file


ALLOWED_EXTENSIONS = set(['webm', 'avi', 'mov', 'mp4', 'mpeg4' 'mkv'])


def allowed_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS


@app.route('/')
def upload_form():
    
    return render_template("index.html")


@app.route('/', methods=['POST'])
def upload_file():
    if request.method == 'POST':
        if 'file' not in request.files:
            flash('No file part')
            return redirect(request.url)
        file = request.files['file']
        if file.filename == '':
            flash('No file selected for uploading')
            return redirect(request.url)
        if file and allowed_file(file.filename):
            filename = secure_filename(file.filename)
            file.save(os.path.join(app.config['INPUT_FOLDER'], filename))
            method = request.form['method']
            process_video(os.path.join(app.config['INPUT_FOLDER'], filename), filename, method)
            return redirect(url_for('view_results'))
        else:
            flash('Allowed file types are webm, avi, mov, mp4, mpeg4, mkv')
            return redirect(request.url)

def process_video(path, filename, method):
    if os.path.exists("vidInfo.json"):
        os.remove("static/output/vidInfo.json")
    image_names = os.listdir("static/output/images")
    for name in image_names:
        os.remove("static/output/images/" + name)
    os.system("./MotionHighlighter -i="+path+" -m="+method)
    os.remove(os.path.join(app.config['INPUT_FOLDER'], filename))

@app.route('/results')
def view_results():
    image_names = os.listdir("static/output/images")
    with open("static/output/vidInfo.json", 'r') as myfile:
        json_data = myfile.read()
    return render_template('output.html', image_names=image_names, json_data=json.dumps(json_data))

@app.route("/download_image/<filename>")
def download_image(filename):
    try:
        return send_from_directory(directory=app.config['OUTPUT_FOLDER'], path=filename, as_attachment=True)
    except FileNotFoundError:
        abort(404)

@app.route("/download_all")
def download_all():
    target1 = 'static/output/images'
    target2 = 'static/output/'

    stream = BytesIO()
    with ZipFile(stream, 'w') as zf:
        for file in glob(os.path.join(target1, '*.jpg')):
            zf.write(file, os.path.basename(file))
        for file in glob(os.path.join(target2, '*.json')):
            zf.write(file, os.path.basename(file))
    stream.seek(0)

    return send_file(
        stream,
        as_attachment=True,
        attachment_filename='archive.zip'
    )

if __name__ == "__main__":
    app.run(debug=True,host= '0.0.0.0',port=5000)