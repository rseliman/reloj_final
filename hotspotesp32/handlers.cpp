#include "WifiCam.hpp"
#include <StreamString.h>
#include <uri/UriBraces.h>

static const char FRONTPAGE[] = R"EOT(


<!DOCTYPE html>
<html lang="es">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SmartWatch Control</title>
    <style>
        body {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
            margin: 0;
        }

        .content {
            display: none;
            text-align: center;
        }

        .content.active {
            display: block;
        }

        .button {
            margin: 10px;
            padding: 20px 60px;
            font-size: 25px;
            cursor: pointer;
        }

        #image-container img {
            width: 90%;
            max-width: 380px;
            margin: 0 auto;
        }

        .slider {
          -webkit-appearance: none;
          width: 100%;
          height: 30px;
          background: #d3d3d3;
          outline: none;
          opacity: 0.7;
          -webkit-transition: .2s;
          transition: opacity .2s;
        }

        .slider:hover {
          opacity: 5;
        }

        .slider::-webkit-slider-thumb {
          -webkit-appearance: none;
          appearance: none;
          width: 40px;
          height: 40px;
          background: #04AA6D;
          cursor: pointer;
        }

        #videoElement {
            width: 80%;
            height: auto;
            display: block;
            margin: 0 auto;
        }

        #videoContainer {
            max-width: 396px;
            max-height: 264px;
            margin: 20px auto 40px auto;
            display: flex;
            justify-content: center;
            align-items: center;
            flex-wrap: wrap;
        }

        #showVideoButton {
            margin-top: 20px;
        }
    </style>
</head>

<body>

    <!-- Página Principal -->
    <div id="SmartWhatchControl" class="content active">
        <h1>SmartWatch Control</h1>
        <button class="button" onclick="showPage('secuencia')">Secuencia</button>
        <button class="button" onclick="showPage('video')">Video</button>
    </div>

    <!-- Página de Secuencia -->
    <div id="secuencia" class="content">
        <!-- Contenido de la subpágina de Secuencia -->
        <h1>Joao Miranda smartwatch Control</h1>
        <button class="button" onclick="loadImages()">Cargar Imágenes</button>
        <div id="image-slider">
            <input type="range" min="0" max="79" value="0" class="slider" id="image-slider-input">
        </div>
        <div id="image-container"></div>
        <button class="button" onclick="showPage('SmartWhatchControl')">Volver</button>
    </div>

 <!-- Página de Video -->
<div id="video" class="content">
    <!-- Contenido de la subpágina de Video -->
    <h1>Video Stream from ESP32-S3 Cam</h1>
    <div id="videoContainer" style="display: none;">
        <img id="videoElement" src="" alt="Video Stream">
    </div>
    <div style="display: flex; justify-content: center; align-items: center;">
        <button class="button" id="showVideoButton">Mostrar Video</button>
        <button class="button" onclick="showPage('SmartWhatchControl')">Volver</button>
    </div>
</div>


<script>
    var totalImages = 80; // Ajusta este valor según la cantidad real de imágenes que tienes en tu secuencia.

    document.addEventListener("DOMContentLoaded", function() {
        var videoContainer = document.getElementById('videoContainer');
        var img = document.getElementById('videoElement');
        var showVideoButton = document.getElementById('showVideoButton');

        var imagesContainer = document.getElementById('image-container'); 
        var imageSliderInput = document.getElementById('image-slider-input');


        showVideoButton.addEventListener('click', function() {
            // Cambiar la fuente de la imagen para mostrar el video
            img.src = 'http://192.168.4.1/800x600.mjpeg?' + new Date().getTime();
            // Mostrar el contenedor de video después de hacer clic en el botón
            videoContainer.style.display = 'block';
            // Ocultar el botón después de mostrar el video
            showVideoButton.style.display = 'none';
        });

        // Opcional: Recargar la imagen cada segundo para obtener un flujo continuo
        // setInterval(function() {
        //     if (videoContainer.style.display === 'block') {
        //         img.src = 'http://192.168.4.1/800x600.mjpeg?' + new Date().getTime();
        //     }
        // }, 1000);
    });

    function showPage(page) {
        var pages = ['SmartWhatchControl', 'secuencia', 'video'];
        for (var i = 0; i < pages.length; i++) {
            var element = document.getElementById(pages[i]);
            if (pages[i] === page) {
                element.classList.add('active');
            } else {
                element.classList.remove('active');
            }
        }
            // Si cambias a otra página que no sea la de video, detén la carga del video
    if (page !== 'video') {
        var videoContainer = document.getElementById('videoContainer');
        var img = document.getElementById('videoElement');
        var showVideoButton = document.getElementById('showVideoButton');
        // Oculta el contenedor de video y muestra el botón de mostrar video
        videoContainer.style.display = 'none';
        showVideoButton.style.display = 'block';
        // Detén la carga del video cambiando la fuente de la imagen
        img.src = '';
    }
}

    function loadImages() {
        var imageSliderInput = document.getElementById('image-slider-input');
        var imagesContainer = document.getElementById('image-container'); 
        imageSliderInput.value = 0;
        imagesContainer.innerHTML = '';

        for (var i = 0; i < totalImages; i++) {
            var imageUrl = `http://192.168.4.1/800x600.jpg?v=${Math.random()}`;
            var imageElement = document.createElement('img');
            imageElement.src = imageUrl;
            imageElement.alt = `Imagen ${i + 1}`;
            imageElement.style.display = 'none';
            imagesContainer.appendChild(imageElement);
        }

        imagesContainer.children[0].style.display = 'block';

        // Lógica para manejar la barra deslizante y mostrar las imágenes según su posición.
        imageSliderInput.addEventListener('input', function () {
            var index = parseInt(this.value);
            for (var i = 0; i < totalImages; i++) {
                imagesContainer.children[i].style.display = 'none';
            }
            imagesContainer.children[index].style.display = 'block';
        });
    }
</script>
       
    </div>

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
      digitalWrite(9, !digitalRead(D9)); // Cambia el estado del LED en el pin D7
      server.send(200, "text/plain", "Toggle LED state"); // Enviar una respuesta al cliente web
  });


server.on("/grabar_cuadro", HTTP_POST, []() {
    // Lógica para grabar el cuadro aquí
    // Agrega la lógica para guardar el cuadro en la estructura de datos
    server.send(200, "text/plain", "Cuadro grabado exitosamente");
});


server.on("/mostrar_cuadros", HTTP_GET, []() {
    // Lógica para mostrar los cuadros grabados aquí
    // Puedes utilizar el array `frames` para obtener las imágenes grabadas
});



  server.on("/", HTTP_GET, [] {0
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
