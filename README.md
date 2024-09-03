# IOT-LLM-Assistant

CloudTranscribtion folder includes files that record a .wav file and using the googlecloud.ino the file is uploaded to the cloud storage and sent to the google Speech to text API.
This folder was not used in the final project.



Project Definition:

LLM assistant named Aura in TAUB faculty of Computer Science.
You can ask about anything! Including specifics about your courses in TAUB!
The current model includes data about Taub in general, the algorithms course and IOT to name a few.

Use Instructions:

Use smartphone to connect to the “LLM Assistant” access point.
Hold hand above sensor to record and remove it to stop.
Check OLED display for messages and instructions.
Different LED states:
-	Solid Red: waiting for WIFI connection (initialization or lost connection)
-	Solid Blue: recording
-	Solid Green: processing

Features:

- Text to Speech
- Speech to Text
- Interaction with OpenAI.
- RAG: retrieves relevant context in local database based on key words in an efficient way using data structures, and formats prompt for OpenAI accordingly
- History: saves last 5 questions and answers with option to reset through “Reset History” or “Delete History” voice command.
- WIFI connection through access point
- Sensor to record audio for a maximum of 15 seconds.
- On/Off Button
- LED to signal different states
- OLED for displaying different states and errors
