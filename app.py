from flask import Flask, render_template, request, jsonify
import mido
import os
from werkzeug.utils import secure_filename
from datetime import datetime
import json

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = 'uploads'
app.config['PROFILE_FOLDER'] = 'static/profiles'
app.config['MAX_CONTENT_LENGTH'] = 16 * 1024 * 1024  # 16MB max file size

# Ensure required folders exist
os.makedirs(app.config['UPLOAD_FOLDER'], exist_ok=True)
os.makedirs(app.config['PROFILE_FOLDER'], exist_ok=True)

# Load profile data
def load_profile():
    profile_path = os.path.join(app.config['PROFILE_FOLDER'], 'profile.json')
    if os.path.exists(profile_path):
        with open(profile_path, 'r') as f:
            return json.load(f)
    return {'name': '', 'description': '', 'picture': None, 'songs': []}

def save_profile(profile_data):
    profile_path = os.path.join(app.config['PROFILE_FOLDER'], 'profile.json')
    with open(profile_path, 'w') as f:
        json.dump(profile_data, f)

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

@app.route('/profile')
def profile():
    profile_data = load_profile()
    return render_template('profile.html', profile=profile_data, songs=profile_data.get('songs', []))

@app.route('/update_profile_picture', methods=['POST'])
def update_profile_picture():
    if 'picture' not in request.files:
        return jsonify({'error': 'No file part'}), 400
    
    file = request.files['picture']
    if file.filename == '':
        return jsonify({'error': 'No selected file'}), 400
    
    if file:
        filename = secure_filename(file.filename)
        filepath = os.path.join(app.config['PROFILE_FOLDER'], filename)
        file.save(filepath)
        
        profile_data = load_profile()
        profile_data['picture'] = f'/static/profiles/{filename}'
        save_profile(profile_data)
        
        return jsonify({'success': True})
    
    return jsonify({'error': 'Invalid file type'}), 400

@app.route('/add_song', methods=['POST'])
def add_song():
    if 'song' not in request.files:
        return jsonify({'error': 'No file part'}), 400
    
    file = request.files['song']
    if file.filename == '':
        return jsonify({'error': 'No selected file'}), 400
    
    if file and file.filename.endswith('.mid'):
        filename = secure_filename(file.filename)
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)
        
        profile_data = load_profile()
        song_data = {
            'id': str(len(profile_data.get('songs', [])) + 1),
            'name': filename,
            'date': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
            'filepath': filepath
        }
        
        if 'songs' not in profile_data:
            profile_data['songs'] = []
        profile_data['songs'].append(song_data)
        save_profile(profile_data)
        
        return jsonify({'success': True})
    
    return jsonify({'error': 'Invalid file type. Please upload a .mid file'}), 400

@app.route('/delete_song/<song_id>', methods=['DELETE'])
def delete_song(song_id):
    profile_data = load_profile()
    songs = profile_data.get('songs', [])
    
    for song in songs:
        if song['id'] == song_id:
            # Remove the file
            if os.path.exists(song['filepath']):
                os.remove(song['filepath'])
            # Remove from profile data
            songs.remove(song)
            profile_data['songs'] = songs
            save_profile(profile_data)
            return jsonify({'success': True})
    
    return jsonify({'error': 'Song not found'}), 404

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