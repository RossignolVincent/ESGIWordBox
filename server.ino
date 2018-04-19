#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <MPU6050_tockn.h>
#include <Wire.h>
#include <Arduino.h>
#ifdef ESP32
  #include <WiFi.h>
  #include "SPIFFS.h"
#else
  #include <ESP8266WiFi.h>
#endif
#include "AudioFileSourceSPIFFS.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"


/* Set these to your desired credentials. */
const char *ssid = "ThreeMenWithoutTeddy";
const char *password = "Azerty1234";
const String audioFilename = "test.ogg";
const String pathAudioFile = "/" + audioFilename;
const String contentTypeAudioFile = "audio/ogg";

/* Sound parameters */
bool speak = false;
// bool mustPlay = false;

int scl = D3; // working value : D3
int sda = D4; // working value : D4

ESP8266WebServer server(80);
File fsUploadFile;
MPU6050 mpu6050(Wire);

AudioGeneratorMP3 *mp3 = NULL;
AudioFileSourceSPIFFS *file;
AudioOutputI2SNoDAC *out;
AudioFileSourceID3 *id3;

String rootHTML = "\
  <!doctype html>\n\
  <html>\n\
    <head>\n\
      <title>IOT</title>\n\
      <meta charest=\"UTF-8\">\n\
    </head>\n\
    <body>\n\
      <div>\n\
        <h2>Audio record and playback</h2>\n\
        <p>\n\
          <button id=startRecord>start</button>\n\
          <button id=stopRecord disabled>stop</button>\n\
        </p>\n\
        <p>\n\
          <audio id=recordedAudio></audio>\n\
          <a id=audioDownload></a>\n\
        </p>\n\
      </div>\n\
      <div>\n\
        <form method=\"post\" enctype=\"multipart/form-data\" action=\"/upload\">\n\
          <input type=\"file\" name=\"name\">\n\
          <input class=\"button\" type=\"submit\" value=\"Upload\">\n\
        </form>\n\
      </div>\n\
      \n\
      <script type='text/javascript'>\n\
        navigator.mediaDevices.getUserMedia({audio:true})\n\
          .then(stream => {\n\
            rec = new MediaRecorder(stream);\n\
            rec.ondataavailable = e => {\n\
              audioChunks.push(e.data);\n\
              \n\
              if (rec.state == 'inactive'){\n\
                let blob = new Blob(audioChunks, {type:'" + contentTypeAudioFile + "'});\n\
                recordedAudio.src = URL.createObjectURL(blob);\n\
                recordedAudio.controls = true;\n\
                recordedAudio.autoplay = true;\n\
                audioDownload.href = recordedAudio.src;\n\
                audioDownload.download = 'ogg';\n\
                audioDownload.innerHTML = 'download';\n\
                \n\
                var formData = new FormData();\n\
                formData.append('file', blob, '" + audioFilename + "');\n\
                \n\
                var request = new XMLHttpRequest();\n\
                request.open('POST', '/audio', true);\n\
                request.onreadystatechange = e => {\n\
                  \n\
                  if(request.readyState == 4 && request.status == 200) {\n\
                    alert(request.responseText);\n\
                  }\n\
                }\n\
                \n\
                request.send(formData);\n\
              }\n\
            }\n\
          })\n\
          .catch(e => console.log(e));\n\
        \n\
        startRecord.onclick = e => {\n\
          startRecord.disabled = true;\n\
          stopRecord.disabled = false;\n\
          audioChunks = [];\n\
          rec.start();\n\
        }\n\
        \n\
        stopRecord.onclick = e => {\n\
          startRecord.disabled = false;\n\
          stopRecord.disabled = true;\n\
          rec.stop();\n\
        }\n\
      </script>\n\
    </body>\n\
  </html>\n\
";

void handleRoot() {
  server.send(200, "text/html", rootHTML);
}

bool handleFileRead() { // send the right file to the client (if it exists)
  // Serial.println("handleFileRead: " + pathAudioFile);
  String contentType = contentTypeAudioFile;             // Get the MIME type
  
  if (SPIFFS.exists(pathAudioFile)) { // If the file exists, either as a compressed archive, or normal
    File file = SPIFFS.open(pathAudioFile, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    // Serial.println(String("\tSent file: ") + pathAudioFile);
    return true;
  }
  
  // Serial.println(String("\tFile Not Found: ") + pathAudioFile);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  
  if(upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if(!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    
    // Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE) {
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END) {
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      // Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/exists");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void ifExists() {
  String path = server.arg("path");
  bool res = SPIFFS.exists(path);
  String message = "le fichier '" + path + "'";
  message += (res) ? " existe" : " n'existe pas";
  
  server.send(200, "text/html", "<h1>coucou " + message + "</h1>");
}

void manageAccelero() {
  mpu6050.update();
  
  float currentZ = mpu6050.getAccZ();
  // Serial.print("-2 2 ");
  // Serial.println(mpu6050.getAccZ());
  
  if (currentZ < -0.5) {
    speak = true;
  } else if (currentZ > 0.5 && speak) {
    speak = false;
    Serial.println("SPEAK");
    // digitalWrite(LED_BUILTIN, HIGH);

    // TODO: start play audio file
    file = new AudioFileSourceSPIFFS("/test.mp3");
    id3 = new AudioFileSourceID3(file);
    id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
    out = new AudioOutputI2SNoDAC();
    mp3 = new AudioGeneratorMP3();
    mp3->begin(id3, out);
  }
}

/*
void manageAudioPlayer() {
  if (mustPlay && mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      mustPlay = false;
      //Serial.println("MP3 done\n");
    }
  } else {
    // Serial.println("MP3 done\n");
  }
}
*/

void setupAccelero() {
  //Serial.println("Wire configuration...");
  Wire.begin(scl, sda);
  
  mpu6050 = MPU6050(Wire);
  mpu6050.begin();
}

// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
  (void)cbData;
  Serial.printf("ID3 callback for: %s = '", type);

  if (isUnicode) {
    string += 2;
  }
  
  while (*string) {
    char a = *(string++);
    if (isUnicode) {
      string++;
    }
    Serial.printf("%c", a);
  }
  Serial.printf("'\n");
  Serial.flush();
}

void handlePlayer() {
  String message = "coucou from handle player";

/*
  file = new AudioFileSourceSPIFFS("/test.mp3");
  id3 = new AudioFileSourceID3(file);
  id3->RegisterMetadataCB(MDCallback, (void*)"ID3TAG");
  out = new AudioOutputI2SNoDAC();
  mp3 = new AudioGeneratorMP3();
  mp3->begin(id3, out);
  */
  
  server.send(200, "text/html", message);
}

void closeMp3() {
  mp3 = NULL;
  id3->close();
  id3 = NULL;
  out = NULL;
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  //Serial.end();

  setupAccelero();
  
  //Serial.println();
  //Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);
  
  IPAddress myIP = WiFi.softAPIP();
  //Serial.print("AP IP address: ");
  //Serial.println(myIP);
  
  SPIFFS.begin();
  
  server.on("/", handleRoot);
  server.on("/download", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead())                                // send it if it exists
      server.send(404, "text/plain", "404: Not Found");   // otherwise, respond with a 404 (Not Found) error
  });
  server.on("/audio", HTTP_POST,
    [](){ server.send(200); },
    handleFileUpload
  );
  server.on("/upload", HTTP_POST,                       // if the client posts to the upload page
    [](){ server.send(200); },                          // Send status 200 (OK) to tell the client we are ready to receive
    handleFileUpload                                    // Receive and save the file
  );
  server.on("/exists", ifExists);
  server.on("/play", handlePlayer);
  
  server.begin();
  //Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  manageAccelero();

  if (mp3 != NULL && mp3->isRunning()) {
    if (!mp3->loop()) {
      mp3->stop();
      Serial.println("close mp3 resources");
      closeMp3();
    }
  } else {
    //Serial.printf("MP3 done\n");
    //delay(1000);
  }
}
