const char MAIN_page[] PROGMEM = R"=====(

<!DOCTYPE html>

<html>

  <head>

    <meta name="viewport" content="width=device-width, initial-scale=1">

    <style>

      html {

        font-family: Arial;

        display: inline-block;

        margin: 0px auto;

        text-align: center;

      }

      

      .slidecontainer {

        width: 100%;

      }

      .slider {

        -webkit-appearance: none;

        width: 50%;

        height: 15px;

        border-radius: 5px;

        background: orange;

        outline: none;

        opacity: 0.5;

        -webkit-transition: .2s;

        transition: opacity .2s;

      }

      .slider:hover {

        opacity: 2;

      }

      .slider::-webkit-slider-thumb {

        -webkit-appearance: none;

        appearance: none;

        width: 25px;

        height: 25px;

        border-radius: 50%;

        background: brown;

        cursor: pointer;

      }

      .slider::-moz-range-thumb {

        width: 25px;

        height: 25px;

        border-radius: 50%;

        background: #4CAF50;

        cursor: pointer;

      }

    </style>

  </head>

  <body>

    <h1> Use Mouse or Left/Right Arrow Keys to Position Blind</h1>
    <h1> For fine tunning use Left/Right Arrow Keys
    <h1> Take note of default values above as these are system generated and can change
    <h1> Click Required Set Position   OPEN  or CLOSED</h1>
    <h1> Click Save when done</h1>
    <h1> Click HOME to de-energise servo's when done</h1>
    
    
    
    

    <br><br>

    <div class="slidecontainer">

      
       <input type="range" min="0" value="90" max="180" class="slider" id="myRange">

      <p>Servo position: <span id="demo"></span></p>

    </div>

    <script>

      function sendData(pos) {

        var xhttp = new XMLHttpRequest();

        xhttp.onreadystatechange = function() {

          if (this.readyState == 4 && this.status == 200) {

            console.log(this.responseText);

          }

        };

        xhttp.open("GET", "setPOS?servoPOS="+pos, true);

        xhttp.send();

      } 

      var slider = document.getElementById("myRange");
      
      var output = document.getElementById("demo");

      output.innerHTML = slider.value;

      slider.oninput = function() {

        output.innerHTML = this.value;

        sendData(output.innerHTML);

      }

    </script>

  </body>

</html>

)=====";

