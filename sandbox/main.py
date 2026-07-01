import paho.mqtt.client as mqtt
import matplotlib.pyplot as plt
from collections import deque
import numpy as np
import time

# --- CONFIGURATION ---
WINDOW_SIZE = 100  # How many time-steps to show on screen at once
NUM_CHANNELS = 8

# Initialize a rolling buffer (queue) for each of the 8 channels
# Deque automatically drops old data when new data arrives if maxlen is set
data_buffers = [deque(maxlen=WINDOW_SIZE) for _ in range(NUM_CHANNELS)]
# Fill them with zeros initially so the plot starts clean
for buf in data_buffers:
    buf.extend([0] * WINDOW_SIZE)

# --- MATPLOTLIB SETUP ---
plt.ion()  # Turn on interactive mode for live updates
fig, axes = plt.subplots(NUM_CHANNELS, 1, figsize=(10, 8), sharex=True)
fig.suptitle("Real-Time Myo Armband sEMG Streams (t0 Channels)", fontsize=14, fontweight='bold')

# Create blank line plots for each channel subplot
lines = []
for i, ax in enumerate(axes):
    line, = ax.plot(np.arange(WINDOW_SIZE), list(data_buffers[i]), color='#2563eb', lw=1.5)
    lines.append(line)
    ax.set_ylabel(f"Ch {i+1}", fontsize=9, rotation=0, labelpad=15)
    ax.set_ylim(-128, 127)  # Set strict limits matching Myo data bounds
    ax.grid(True, alpha=0.3)

plt.xlabel("Time Samples (Rolling Window)")
plt.tight_layout(rect=[0, 0, 1, 0.95])

# --- MQTT CALLBACKS ---
def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("Visualizer connected to Mosquitto successfully!")
        client.subscribe("esp32/myo/emg")
    else:
        print(f"Connection failed: {rc}")

def on_message(client, userdata, msg):
    try:
        payload_str = msg.payload.decode('utf-8').strip()
        data_points = payload_str.split(' ')
        
        if len(data_points) == 16:
            # Convert string to signed integers
            emg_values = [int(val) for val in data_points]
            
            # Extract just the first time step (t0: indices 0 to 7) for real-time tracking
            t0_channels = emg_values[0:8]
            
            # Push the new numbers into our rolling queues
            for i in range(NUM_CHANNELS):
                data_buffers[i].append(t0_channels[i])
                
    except Exception as e:
        pass  # Silently skip malformed network packets to avoid stopping the graph

# --- RUNNING THE SANDBOX ---
client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message

client.connect("localhost", 1883, 60)
client.loop_start()  # Run MQTT network thread in the background

print("Displaying Live UI. Close the window or press Ctrl+C to stop.")
try:
    while True:
        # Update the data data arrays for each graphical line
        for i in range(NUM_CHANNELS):
            lines[i].set_ydata(list(data_buffers[i]))
        
        # Redraw the canvas cleanly
        fig.canvas.draw()
        fig.canvas.flush_events()
        
        # Tight control loop sleep to match your ~200Hz stream frequency
        time.sleep(0.01)

except KeyboardInterrupt:
    print("\nClosing Lab Visualizer...")
    client.loop_stop()
    plt.close('all')