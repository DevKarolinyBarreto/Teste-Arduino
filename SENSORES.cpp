#include <Arduino.h>
#include <DHT.h>
#include <WiFi.h>
#include <time.h>

#define DHTPIN 4
#define DHTTYPE DHT11

#define LDRPIN 12

const char *ssid = "IoT-Senac";
const char *password = "Senac@2025";

const char *ntpServer = "pool.ntp.org";

const long gmtOffset_sec = -10800;
const int daylightOffset_sec = 0;

//int luminosidade = A12;

WiFiServer server(80);
DHT dht(DHTPIN, DHTTYPE);

String getDateTime();
void handleClientRequest(WiFiClient &client, float temperatura, float umidade, int luminosidade);
void sendHttpResponse(WiFiClient &client, float temperatura, float umidade, int luminosidade, String dateTime);

void setup()
{
  Serial.begin(115200);
  Serial.print("Conectando-se a: ");
  Serial.println(ssid);
  pinMode(LDRPIN, INPUT);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wifi conectada.");
  Serial.println("Endereço de IP de acesso de qualquer browser: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  dht.begin();
  delay(1000);
  server.begin();
}

void loop()
{
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();
  int ldrvalor = analogRead(LDRPIN);
  Serial.print("Luminosidade recebida: ");
  Serial.println(ldrvalor);

  WiFiClient client = server.available();

  if (client)
  {
    //("Nova requisição.");
    handleClientRequest(client, temperatura, umidade, ldrvalor);
    client.stop();
  }
}

String getDateTime()
{
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Falha ao obter o tempo");
    return "Erro ao obter hora";
  }

  char buffer[30];

  strftime(buffer, sizeof(buffer), "%d/%m/%Y %H:%M:%S", &timeinfo);
  return String(buffer);
}

void handleClientRequest(WiFiClient &client, float temperatura, float umidade, int luminosidade)
{
  String currentLine = "";

  while (client.connected())
  {
    if (client.available())
    {
      char c = client.read();
      Serial.write(c);
      if (c == '\n')
      {
        if (currentLine.length() == 0)
        {
          String dateTime = getDateTime();
          sendHttpResponse(client, temperatura, umidade, luminosidade, dateTime);
          break;
        }
        else
        {
          currentLine = "";
        }
      }
      else if (c != '\r')
      {
        currentLine += c;
      }
    }
  }
}

void sendHttpResponse(WiFiClient &client, float temperatura, float umidade, int luminosidade, String dateTime)
{
  // Cabeçalhos HTTP
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  // HTML
  client.println("<!DOCTYPE html>");
  client.println("<html lang='pt-BR'>");
  client.println("<head>"
                 "<meta charset='UTF-8'>"
                 "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                 "<meta http-equiv='refresh' content='60'>"
                 "<title>Monitoramento ESP32 com DHT 11</title>"

                 "<link href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/5.15.4/css/all.min.css' rel='stylesheet'>"

                 "<style>"
                 "body {"
                 "font-family: Arial, sans-serif;"
                 "text-align: center;"
                 "background-color: #e3e3e3;"
                 "margin: 0;"
                 "padding: 0;"
                 "}"

                 ".container {"
                 "max-width: 90%;"
                 "width: 380px;"
                 "margin: 20px auto;"
                 "padding: 20px;"
                 "background: white;"
                 "box-shadow: 4px 4px 10px rgba(0, 0, 0, 0.1);"
                 "border-radius: 12px;"
                 "}"

                 ".logo {"
                 "width: 50%;"
                 "max-width: 120px;"
                 "display: block;"
                 "margin: 0 auto 15px;"
                 "}"

                 "h1 {"
                 "font-size: 20px;"
                 "color: #333;"
                 "margin-bottom: 10px;"
                 "}"

                 ".data-section {"
                 "display: flex;"
                 "align-items: center;"
                 "justify-content: center;"
                 "margin: 10px 0;"
                 "}"

                 ".icon {"
                 "font-size: 24px;"
                 "margin-right: 10px;"
                 "}"

                 ".temp-icon {"
                 "color: #ff5733;"
                 "}"

                 ".umidade-icon {"
                 "color: #1e90ff;"
                 "}"

                 ".info-box {"
                 "background: #f8f8f8;"
                 "padding: 10px;"
                 "border-radius: 8px;"
                 "margin: 5px 0;"
                 "display: flex;"
                 "align-items: center;"
                 "justify-content: center;"
                 "font-size: 18px;"
                 "font-weight: bold;"
                 "}"
                 "</style>"

                 "</head>");

  client.println("<body>"

                 "<div class='container'>"
                 "<img src='https://i0.wp.com/centralblogs.com.br/wp-content/uploads/2020/09/senac-logo.png?w=696&ssl=1'"
                 "alt='Logo Senac' class='logo'>"
                 "<div class='info-box'>"
                 "<h1 style = 'color: #0056b3;' > Taboão da Serra </h1>"
                 "</div>"
                 "<h1>Turma IoT - Tarde</h1>"
                 "<h1>Dados do Sensor DHT11</h1>"

                 "<div class='info-box'>"
                 "<i class='fas fa-thermometer-half icon temp-icon'></i>");
  // Temperatura
  client.print("Temperatura: ");
  client.print(temperatura);
  client.println(" ºC</div>");

  // Umidade
  client.println("<div class='info-box'>"
                 "<i class='fas fa-tint icon umidade-icon'></i>"
                 "Umidade: ");
  client.print(umidade);
  client.println(" %</div>");

  // Luminosidade
  client.print("Luminosidade: ");
  client.print(luminosidade);
  client.println(" %</div>");

  // Última atualização
  client.println("<p> <strong> Última Atualização : </strong>" + dateTime + "</p>"
                                                                            "</div>"
                                                                            "</body>"
                                                                            "</html>");
}