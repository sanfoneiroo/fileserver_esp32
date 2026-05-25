# ESP32 File Server

Servidor de arquivos embarcado para ESP32 com:

- suporte a LittleFS
- suporte a cartão microSD
- gerenciador de arquivos via navegador
- display OLED para status do sistema
- upload / download / renomeação / exclusão de arquivos
- hospedagem de arquivos estáticos
- abstração unificada de múltiplos sistemas de arquivos

Projetado como um pequeno appliance de armazenamento e servidor web estático para redes locais.

---

# Funcionalidades

## Interface Web

- upload de arquivos
- download de arquivos
- abertura de arquivos no navegador
- renomeação de arquivos
- exclusão de arquivos
- formatação do LittleFS
- exibição de uso do armazenamento

---

## Sistemas de Arquivos

### LittleFS
Armazenamento interno em flash.

### Cartão SD
Armazenamento externo via SPI.

O sistema abstrai ambos os sistemas de arquivos utilizando os mesmos handlers e rotas.

---

## Display OLED

Exibe:

- endereço IP do WiFi
- uso do LittleFS
- uso do cartão SD
- mensagens de log do sistema

---

# Hospedagem de Arquivos Estáticos

Rotas desconhecidas são automaticamente procuradas em:

1. LittleFS
2. cartão SD

Isso permite que o ESP32 funcione como um pequeno servidor web estático.

Exemplo:

```text
/index.html
/style.css
/app.js
```

podem ser servidos diretamente pelo sistema de arquivos.

---

# Rotas Web

| Rota | Função |
|---|---|
| `/` | Interface principal |
| `/open` | Abrir arquivo no navegador |
| `/download` | Baixar arquivo |
| `/upload` | Enviar arquivo |
| `/rename` | Renomear arquivo |
| `/delete` | Excluir arquivo |
| `/format` | Formatar LittleFS |

---

# Hardware

## Testado com

- ESP32 Dev Module
- OLED SSD1306 (I2C)
- módulo microSD SPI

---

# Ligações do Cartão SD

Exemplo de conexão SPI:

| SD Module | ESP32 |
|---|---|
| MISO | GPIO 19 |
| MOSI | GPIO 23 |
| SCK | GPIO 18 |
| CS | GPIO 5 |

Os pinos podem ser alterados em `config.h`.

---

# Ligações do OLED

Exemplo de conexão I2C:

| OLED | ESP32 |
|---|---|
| SDA | GPIO 21 |
| SCL | GPIO 22 |

Os pinos podem ser alterados em `config.h`.

---

# Dependências

Bibliotecas Arduino utilizadas:

- WiFi
- WebServer
- LittleFS
- SD
- SPI
- Wire
- Adafruit GFX
- Adafruit SSD1306

Instale utilizando o Arduino Library Manager.

---

# Configuração

## wifi_config.h

Exemplo:

```cpp
#pragma once

namespace WiFiConfig
{
  void begin()
  {
    WiFi.begin(
      "SEU_WIFI",
      "SUA_SENHA"
    );

    while (
      WiFi.status() != WL_CONNECTED
    )
    {
      delay(500);
    }
  }
}
```

---

## config.h

Exemplo:

```cpp
#pragma once

// OLED

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_ADDRESS 0x3C

#define OLED_SDA_PIN 21
#define OLED_SCL_PIN 22

// SD SPI

#define SD_SCK 18
#define SD_MISO 19
#define SD_MOSI 23
#define SD_CS 5
```

---

# Utilização

Após iniciar:

1. conecte o ESP32 ao WiFi
2. leia o endereço IP no OLED
3. abra o navegador
4. acesse:

```text
http://IP_DO_ESP32
```

---

# Observações

## Renomeação de Arquivos

A renomeação é implementada utilizando:
- cópia
- criação de novo arquivo
- remoção do original

em vez de utilizar renomeação nativa do sistema de arquivos.

Isso melhora a compatibilidade entre LittleFS e SD.

---

## Subdiretórios

A versão atual é focada em armazenamento simples na raiz.

Navegação recursiva e gerenciamento completo de subdiretórios ainda não estão implementados.

---

# Objetivos do Projeto

O projeto busca priorizar:

- simplicidade
- legibilidade
- baixo número de dependências
- ferramentas web embarcadas
- hospedagem leve de arquivos
- arquitetura didática para sistemas embarcados

---

# Possíveis Melhorias Futuras

- suporte completo a subdiretórios
- upload drag-and-drop
- CSS embarcado
- autenticação
- preview de arquivos
- geração HTML por streaming
- servidor assíncrono
- download ZIP

---

# Licença

MIT