"""
Simple Flask API server for ESP32 sensor data
Run this alongside the sensor_display.html to receive and serve data from your ESP32

Usage:
    1. Install dependencies: pip install flask flask-cors
    2. Run this file: python app.py
    3. Configure your ESP32 to POST data to http://your-machine-ip:5000/api/data
    4. Open sensor_display.html and set API URL to: http://your-machine-ip:5000/api/data
"""

from flask import Flask, request, jsonify
from flask_cors import CORS
from datetime import datetime
import json

app = Flask(__name__)
CORS(app)

# Store latest sensor data
current_data = {
    'ph': 7.0,
    'ec': 0,
    'timestamp': datetime.now().isoformat()
}

# Store historical data (last 1000 readings)
data_history = []
MAX_HISTORY = 1000


@app.route('/api/data', methods=['GET', 'POST'])
def handle_data():
    """
    GET: Returns the latest sensor reading
    POST: Receives new sensor data from ESP32
    
    Expected JSON format:
    {
        "ph": 7.25,
        "ec": 1500
    }
    """
    global current_data
    
    if request.method == 'POST':
        try:
            data = request.get_json()
            
            # Validate data
            if 'ph' not in data or 'ec' not in data:
                return jsonify({'error': 'Missing pH or EC value'}), 400
            
            # Update current data
            current_data = {
                'ph': float(data['ph']),
                'ec': float(data['ec']),
                'timestamp': datetime.now().isoformat()
            }
            
            # Add to history
            data_history.append(current_data.copy())
            if len(data_history) > MAX_HISTORY:
                data_history.pop(0)
            
            print(f"[{current_data['timestamp']}] pH: {current_data['ph']}, EC: {current_data['ec']}")
            
            return jsonify({
                'status': 'success',
                'message': 'Data received',
                'data': current_data
            }), 200
            
        except Exception as e:
            return jsonify({'error': str(e)}), 400
    
    # GET request
    return jsonify(current_data), 200


@app.route('/api/history', methods=['GET'])
def get_history():
    """Returns historical sensor data"""
    return jsonify(data_history), 200


@app.route('/api/clear-history', methods=['POST'])
def clear_history():
    """Clears historical data"""
    global data_history
    data_history = []
    return jsonify({'status': 'success', 'message': 'History cleared'}), 200


@app.route('/health', methods=['GET'])
def health():
    """Health check endpoint"""
    return jsonify({'status': 'ok'}), 200


if __name__ == '__main__':
    print("Starting pH/EC Sensor API Server...")
    print("Server running at http://localhost:5000")
    print("API endpoint: POST to http://localhost:5000/api/data")
    print("\nExpected JSON format:")
    print('{"ph": 7.25, "ec": 1500}')
    print("\nPress Ctrl+C to stop the server")
    app.run(debug=True, host='0.0.0.0', port=5000)
