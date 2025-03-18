from flask import Flask, render_template, request, jsonify
import mido
import os
from werkzeug.utils import secure_filename

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = 'uploads'
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB max file size

# Ensure upload folder exists
os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)

def midi_to_string_list(midi_file):
    mid = mido.MidiFile(midi_file)
    notes = []
    
    for track in mid.tracks:
        for msg in track:
            if msg.type == 'note_on' and msg.velocity > 0:
                notes.append(f"Note: {msg.note}, Velocity: {msg.velocity}, Time: {msg.time}")
            elif msg.type == 'note_off':
                notes.append(f"Note Off: {msg.note}, Time: {msg.time}")
    
    return notes

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/upload', methods=['POST'])
def upload_file():
    if 'file' not in request.files:
        return jsonify({'error': 'No file part'}), 400
    
    file = request.files['file']
    if file.filename == '':
        return jsonify({'error': 'No selected file'}), 400
    
    if file and file.filename.endswith('.mid'):
        filename = secure_filename(file.filename)
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)
        
        try:
            notes = midi_to_string_list(filepath)
            os.remove(filepath)  # Clean up the uploaded file
            return jsonify({'notes': notes})
        except Exception as e:
            return jsonify({'error': str(e)}), 500
    
    return jsonify({'error': 'Invalid file type. Please upload a .mid file'}), 400

if __name__ == '__main__':
    app.run(debug=True) 