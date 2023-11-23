#include "WifiCam.hpp"
#include <StreamString.h>
#include <uri/UriBraces.h>

static const char FRONTPAGE[] = R"EOT(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Image and Video Viewer</title>
    <style>
        body {
            margin: 0;
            padding: 0;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
            background-color: #f0f0f0;
        }

        #mediaContainer {
            width: 800px;
            height: 600px;
            overflow: hidden;
            display: flex;
            justify-content: center;
            align-items: center;
        }

        #mediaContainer iframe, #mediaContainer img {
            width: 800px;
            height: 600px;
        }

        #controls {
            display: flex;
            gap: 20px;
            margin-bottom: 20px;
        }

        button {
            padding: 10px 20px;
            font-size: 18px;
        }
    </style>
</head>

<body>
    <div id="controls">
        <button id="captureImageButton">Foto</button>
        <button id="startVideoButton">Video</button>
        <button id="captureSequenceButton">Secuencia</button>
        <button id="prevImageButton">Anterior</button>
        <button id="nextImageButton">Siguiente</button>
    </div>
    <div id="mediaContainer"></div>

    <script>
        var mediaContainer = document.getElementById('mediaContainer');
        var captureImageButton = document.getElementById('captureImageButton');
        var startVideoButton = document.getElementById('startVideoButton');
        var captureSequenceButton = document.getElementById('captureSequenceButton');
        var prevImageButton = document.getElementById('prevImageButton');
        var nextImageButton = document.getElementById('nextImageButton');
        var currentIndex = 0;
        var images = [];

        function loadImages() {
            mediaContainer.innerHTML = '';
            if (images.length > 0) {
                mediaContainer.appendChild(images[currentIndex]);
            } else {
                var message = document.createElement('p');
                message.textContent = 'No hay imágenes para mostrar.';
                mediaContainer.appendChild(message);
            }
        }

        function captureImage() {
                     try {
                        images = [];
                        var timestamp = new Date().getTime(); // Marca de tiempo actual
                        var img = new Image();
                        img.src = 'http://192.168.4.1/800x600.jpg?' + timestamp;
                        img.onload = function() {
                            images.push(img);
                            loadImages();
                            // Vuelve a vincular el evento click después de cargar la nueva imagen
                            captureImageButton.addEventListener('click', captureImage);
                         };
                      } catch (error) {
                       console.error('Error al capturar la imagen:', error);
                        }
                  }


        captureImageButton.addEventListener('click', captureImage);

        startVideoButton.addEventListener('click', function() {
            images = [];
            var iframe = document.createElement('iframe');
            iframe.src = 'http://192.168.4.1/800x600.mjpeg?width=800&height=600';
            mediaContainer.innerHTML = '';
            mediaContainer.appendChild(iframe);
        });

        captureSequenceButton.addEventListener('click', function() {
            images = [];
            for (var i = 0; i < 30; i++) {
                var img = new Image();
                img.src = 'http://192.168.4.1/800x600.jpg?id=' + (i + 1) + '&' + new Date().getTime();
                images.push(img);
            }
            loadImages();
        });

        prevImageButton.addEventListener('click', function() {
            if (currentIndex > 0) {
                currentIndex--;
                loadImages();
            }
        });

        nextImageButton.addEventListener('click', function() {
            if (currentIndex < images.length - 1) {
                currentIndex++;
                loadImages();
            }
        });
    </script>
</body>

</html>

    )EOT";

static void
serveStill(bool wantBmp)
{
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("capture() failure");
    server.send(500, "text/plain", "still capture error\n");
    return;
  }
  Serial.printf("capture() success: %dx%d %zub\n", frame->getWidth(), frame->getHeight(),
                frame->size());

  if (wantBmp) {
    if (!frame->toBmp()) {
      Serial.println("toBmp() failure");
      server.send(500, "text/plain", "convert to BMP error\n");
      return;
    }
    Serial.printf("toBmp() success: %dx%d %zub\n", frame->getWidth(), frame->getHeight(),
                  frame->size());
  }

  server.setContentLength(frame->size());
  server.send(200, wantBmp ? "image/bmp" : "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}

static void
serveMjpeg()
{
  Serial.println("MJPEG streaming begin");
  WiFiClient client = server.client();
  auto startTime = millis();
  int nFrames = esp32cam::Camera.streamMjpeg(client);
  auto duration = millis() - startTime;
  Serial.printf("MJPEG streaming end: %dfrm %0.2ffps\n", nFrames, 1000.0 * nFrames / duration);
}

void
addRequestHandlers()
{

  server.on("/toggle_led", HTTP_GET, []() {
      digitalWrite(D9, !digitalRead(D9)); // Cambia el estado del LED en el pin D7
      server.send(200, "text/plain", "Toggle LED state"); // Enviar una respuesta al cliente web
  });



  

  server.on("/", HTTP_GET, [] {
    server.setContentLength(sizeof(FRONTPAGE));
    server.send(200, "text/html");
    server.sendContent(FRONTPAGE, sizeof(FRONTPAGE));
  });

  server.on("/robots.txt", HTTP_GET,
            [] { server.send(200, "text/html", "User-Agent: *\nDisallow: /\n"); });

  server.on("/resolutions.csv", HTTP_GET, [] {
    StreamString b;
    for (const auto& r : esp32cam::Camera.listResolutions()) {
      b.println(r);
    }
    server.send(200, "text/csv", b);
  });

  server.on(UriBraces("/{}x{}.{}"), HTTP_GET, [] {
    long width = server.pathArg(0).toInt();
    long height = server.pathArg(1).toInt();
    String format = server.pathArg(2);
    if (width == 0 || height == 0 || !(format == "bmp" || format == "jpg" || format == "mjpeg")) {
      server.send(404);
      return;
    }

    auto r = esp32cam::Camera.listResolutions().find(width, height);
    if (!r.isValid()) {
      server.send(404, "text/plain", "non-existent resolution\n");
      return;
    }
    if (r.getWidth() != width || r.getHeight() != height) {
      server.sendHeader("Location",
                        String("/") + r.getWidth() + "x" + r.getHeight() + "." + format);
      server.send(302);
      return;
    }

    if (!esp32cam::Camera.changeResolution(r)) {
      Serial.printf("changeResolution(%ld,%ld) failure\n", width, height);
      server.send(500, "text/plain", "changeResolution error\n");
    }
    Serial.printf("changeResolution(%ld,%ld) success\n", width, height);

    if (format == "bmp") {
      serveStill(true);
    } else if (format == "jpg") {
      serveStill(false);
    } else if (format == "mjpeg") {
      serveMjpeg();
    }
  });
}
