The main purpose of this prototype is to demonstrate the inter-connectivity of devices in wireless medium. Additionally, the features of the 'smart mobility cane' is also implemented as proof of concept.

# System Architecture
<img src="../res/smart cane system.jpg" height="80%" width="80%"/>

Voice Input and GPS Coordination (Xiao-3):
-
- Voice Command Integration: The system starts by capturing user input through Google Assistant. The user provides voice commands to specify the source and destination locations.
- Fetching GPS Coordinates: Using the Google Assistant API, Xiao-3 retrieves the GPS coordinates for both the source and destination.
- Route Generation: With the acquired coordinates, Xiao-3 uses the Google Maps API to generate a detailed route. This route is then transmitted wirelessly to the Master Xiao.
- Real-Time GPS Tracking: Xiao-3 continuously monitors the user's real-time GPS coordinates and sends updates to the Master Xiao.

Motion Sensing (Xiao-2):
-
- Gyroscope and Accelerometer Data: Xiao-2 is equipped with an MPU6050 sensor to gather acceleration and gyroscope data. This data helps in determining the user’s movements and orientation.
- Wireless Communication: Xiao-2 sends the processed motion data wirelessly to the Master Xiao.

Central Control and User Guidance (Master Xiao):
-
- Data Integration and Processing: The Master Xiao receives the route information from Xiao-3 and the motion data from Xiao-2. It integrates these inputs to understand the user’s current position and movement.
- User Indication: Based on the processed data, the Master Xiao provides navigation instructions to the user. This could be through auditory, visual, or haptic feedback, ensuring the user stays on the correct path.

Key Technologies and APIs
-
- Microcontrollers: Xiao-2, Xiao-3, and Master Xiao.
- Sensors: MPU6050 (accelerometer and gyroscope) for motion detection.
- APIs: Google Assistant API for voice input and GPS coordination; Google Maps API for route generation.
- Wireless Communication: EPS-NOW protocol.

Future Scope
-
The main goal of this project is to create a fully equipped mobility cane for the blind. The future of this project is to scale down the electronic components and create a robust device, which will solve all the issues pointed out in the beginning. 
- Firstly, a single brain [microcontroller] will be used.
- many additional sensors will be added, like; proximity, ultrasonic, bluetooth etc.
- unique feedback systems will be implemented to adapt the surroundings.
- More real time testing will take place
