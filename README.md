Arduino sketch designed to gather data from various sensors (temperature, voltage, current, and irradiation), store it locally on an SD card, and upload it to an FTP server. Here's a summary of its functionality:

Libraries: It includes several libraries for handling different tasks such as temperature sensing, SD card operations, time handling, WiFi connection, and FTP client operations.

Pin Declarations: Defines pin numbers for various sensors, relays, and other peripherals connected to the Arduino.

Variable Declarations: Defines variables for storing sensor readings, flags for indicating sensor status, and other necessary variables.

Functions:

ConnectWiFi(): Connects to a WiFi network.
updateTime(): Retrieves current time from an NTP server.
ftpClient(): Handles FTP client operations to upload data to a remote server.
sdcard(): Handles SD card operations to store data locally.
createDir(), writeFile(), appendFile(): Functions to manage file operations on the SD card.
tempCheck(), tempFunc(): Functions to read temperature from multiple sensors.
voltageFunction(), currentFunction(): Functions to read voltage and current from sensors.
printAddress(): Function to print the address of temperature sensors.
Setup(): Initializes various components, connects to WiFi, configures pins, begins communication with sensors, sets up time handling, and initializes the SD card.

Loop(): The main loop of the program which continuously reads sensor data, updates time, manages WiFi connection, and performs actions based on certain conditions such as switching relays on or off based on time, and periodically uploading data to the FTP server and storing it on the SD card.
