import os
import urllib.request
from app import app
from flask import Flask, flash, request, redirect, render_template, url_for, send_from_directory
from werkzeug.utils import secure_filename

ALLOWED_EXTENSIONS = set(['webm', 'avi', 'mov', 'mp4', 'mpeg4' 'mkv'])


def allowed_file(filename):
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS


@app.route('/')
def upload_form():
    image_names = os.listdir("static/output/images")
    for name in image_names:
        os.remove("static/output/images/" + name)
    return render_template("index.html")


@app.route('/', methods=['POST'])
def upload_file():
    if request.method == 'POST':
        # check if the post request has the file part
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
            process_video(os.path.join(app.config['INPUT_FOLDER'], filename), filename)
            os.remove(os.path.join(app.config['INPUT_FOLDER'], filename))
            return redirect(url_for('view_results'))
        else:
            flash('Allowed file types are webm, avi, mov, mp4, mpeg4, mkv')
            return redirect(request.url)

def process_video(path, filename):
     os.system("./MotionHighlighter -i="+path+" -m=1")

def delete_file(path, filename):
    os.remove(os.path.join(path, filename))

@app.route('/results')
def view_results():
 image_names = os.listdir("static/output/images")
 print(image_names)
 return render_template('output.html', image_names=image_names)

@app.route("/download_image/<filename>")
def download_image(filename):
    try:
        return send_from_directory(directory=app.config['OUTPUT_FOLDER'], path=filename, as_attachment=True)
    except FileNotFoundError:
        abort(404)


# @app.route('/uploads/<filename>')
# def uploaded_file(filename):
#     return send_from_directory(app.config['OUTPUT_FOLDER'], filename, as_attachment=True)

if __name__ == "__main__":
    app.run(debug=True,host= '0.0.0.0',port=5000)