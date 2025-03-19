from flask import Flask, render_template, request, jsonify
import mido
import os
from werkzeug.utils import secure_filename
from datetime import datetime
import json
import sys

app = Flask(__name__)
app.config['UPLOAD_FOLDER'] = 'uploads'
app.config['PROFILE_FOLDER'] = 'static/profiles'
# Increase max file size to 500MB
app.config['MAX_CONTENT_LENGTH'] = 500 * 1024 * 1024  # 500MB max file size

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
    try:
        mid = mido.MidiFile(midi_file)
        notes = []
        total_notes = 0
        max_notes = 10000  # Limit the number of notes to prevent memory issues
        
        for track in mid.tracks:
            for msg in track:
                if total_notes >= max_notes:
                    notes.append(f"... (Showing first {max_notes} notes)")
                    return notes
                    
                if msg.type == 'note_on' and msg.velocity > 0:
                    notes.append(f"Note: {msg.note}, Velocity: {msg.velocity}, Time: {msg.time}")
                    total_notes += 1
                elif msg.type == 'note_off':
                    notes.append(f"Note Off: {msg.note}, Time: {msg.time}")
                    total_notes += 1
        
        return notes
    except Exception as e:
        raise Exception(f"Error processing MIDI file: {str(e)}")

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
        try:
            filename = secure_filename(file.filename)
            filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
            file.save(filepath)
            
            # Verify the file is a valid MIDI file
            mid = mido.MidiFile(filepath)
            
            profile_data = load_profile()
            song_data = {
                'id': str(len(profile_data.get('songs', [])) + 1),
                'name': filename,
                'date': datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                'filepath': filepath,
                'size': os.path.getsize(filepath),
                'duration': sum(msg.time for msg in mid.tracks[0]) if mid.tracks else 0
            }
            
            if 'songs' not in profile_data:
                profile_data['songs'] = []
            profile_data['songs'].append(song_data)
            save_profile(profile_data)
            
            return jsonify({'success': True})
        except Exception as e:
            if os.path.exists(filepath):
                os.remove(filepath)
            return jsonify({'error': f'Invalid MIDI file: {str(e)}'}), 400
    
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
        try:
            filename = secure_filename(file.filename)
            filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
            file.save(filepath)
            
            # Verify the file is a valid MIDI file
            mid = mido.MidiFile(filepath)
            
            try:
                # Get file information before processing
                file_info = {
                    'name': filename,
                    'size': os.path.getsize(filepath),
                    'duration': sum(msg.time for msg in mid.tracks[0]) if mid.tracks else 0
                }
                
                # Process the MIDI file
                notes = midi_to_string_list(filepath)
                
                # Clean up the uploaded file
                if os.path.exists(filepath):
                    os.remove(filepath)
                
                return jsonify({
                    'notes': notes,
                    'file_info': file_info
                })
            except Exception as e:
                if os.path.exists(filepath):
                    os.remove(filepath)
                return jsonify({'error': str(e)}), 500
                
        except Exception as e:
            if os.path.exists(filepath):
                os.remove(filepath)
            return jsonify({'error': f'Invalid MIDI file: {str(e)}'}), 400
    
    return jsonify({'error': 'Invalid file type. Please upload a .mid file'}), 400

if __name__ == '__main__':
    app.run(debug=True) 