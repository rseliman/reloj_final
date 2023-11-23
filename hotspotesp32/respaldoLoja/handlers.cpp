#include "WifiCam.hpp"
#include <StreamString.h>
#include <uri/UriBraces.h>

static const char FRONTPAGE[] = R"EOT(
<!doctype html>
<title>Joao Miranda Smartwatch Control</title>
    <style>
        body {
            text-align: center;
        }

        .button {
            margin: 10px;
            padding: 15px 30px;
            font-size: 18px;
            cursor: pointer;
        }

        #image-container {
            margin-top: 20px;
        }

        #video-container {
            margin-top: 20px;
            width: 800px; /* Ancho del video */
            height: 600px; /* Altura del video */
            max-width: 100%; /* Garantiza que el video no sea más grande que el contenedor padre */
            max-height: 100%; /* Garantiza que el video no sea más alto que el contenedor padre */
        }
    </style>
</head>

<body>
    <h1>Joao Miranda Smartwatch Control</h1>
    <button class="button" onclick="showImage('/800x600.jpg')">Mostrar Imagen</button>
    <button class="button" onclick="showVideo('/800x600.mjpeg')">Mostrar Video</button>
    <button class="button" onclick="toggleLed()">On/Off Led</button>
    
    <div id="image-container"></div>
    <img id="video-container" src="" alt="Video Stream">

    <script>
        function showImage(imageUrl) {
            imageUrl += `?${Math.random()}`;
            document.getElementById('image-container').innerHTML = `<img src="${imageUrl}" alt="Imagen 800x600">`;
            document.getElementById('video-container').src = ''; // Detener el flujo de video si está reproduciéndose
        }

        function showVideo(videoUrl) {
            videoUrl += `?${Math.random()}`;
            document.getElementById('image-container').innerHTML = ''; // Limpiar el contenedor de imagen
            document.getElementById('video-container').src = videoUrl; // Establecer la fuente del flujo MJPEG
        }
        function toggleLed() {
            var xhr = new XMLHttpRequest();
            xhr.open("GET", "/toggle_led", true); // Enviar una solicitud GET a "/toggle_led" en el servidor
            xhr.send();

            // La respuesta del servidor no se maneja en este ejemplo, pero podrías manejarla si es necesario
        }
    </script>
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
