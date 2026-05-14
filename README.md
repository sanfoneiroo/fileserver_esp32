O projeto apresenta um sistema embarcado baseado em ESP32 que implementa um servidor de arquivos acessível via navegador web, permitindo operações de upload, download, listagem e gerenciamento de arquivos em uma interface simples e responsiva.

O sistema utiliza o sistema de arquivos LittleFS como um dos recursos de armazenamento interno, garantindo estabilidade, baixa fragmentação e acesso direto a arquivos via HTTP. Em conjunto, a arquitetura prevê suporte a armazenamento externo via cartão microSD, permitindo expansão da capacidade de armazenamento e separação entre arquivos de sistema e arquivos do usuário.

A arquitetura do sistema foi projetada como um serviço de rede autônomo, no qual o ESP32 atua como servidor HTTP responsável por expor uma interface web de gerenciamento de arquivos e endpoints de acesso direto. A comunicação ocorre inteiramente via requisições HTTP, permitindo interação com o sistema através de qualquer navegador sem dependência de software externo.

A configuração de rede é realizada por meio de um sistema de WiFi Manager embarcado, permitindo que o dispositivo seja inicialmente configurado em modo de acesso (AP), com persistência de credenciais em memória não volátil (NVS). Após a configuração, o sistema realiza conexão automática em modo STA, garantindo reconexão transparente às redes previamente configuradas.
