# PBL3-SE
Projeto de embarcados para monitoramento de ambiente. ESP32 capta os dados dos sensores (temperatura, umidade, nível de gás e luminosidade) e, com base nestes dados, utiliza uma multilayer perceptron treinada para calcular as condições do ambiente. Os dados dos sensores e a condição ambiental são enviados para uma dashboard remota via MQTT.

__Alunos:__
Lucas Carneiro de Araújo Lima e  Ian Zaque Santos

## ESP-CAM
Repositório do projeto que utiliza a ESPCAM para capturar a foto do ambiente sob condições ambientais ideais: [PBL3-SE-ESPCAM](https://github.com/LucasCarneiro3301/PBL3-SE-ESPCAM.git)

## API para banco de dados
Repositório do projeto que capta os dados dos sensores e condições do ambiente e salva em um banco de dados: [PBL3-SE-API](https://github.com/ian-zaque/SE-PBL3-API.git)

## Instruções de Uso

### 1. Clone o repositório
Abra o terminal e execute o comando abaixo para clonar o repositório em sua máquina:
```bash
https://github.com/LucasCarneiro3301/PBL3-SE.git
```

### 2. Configure o ambiente de desenvolvimento
Certifique-se de que a extensão [Platform.io](https://platformio.org/install/ide?install=vscode) esteja instalada e configurada no ambiente do VS Code.

### 3. Observações (IMPORTANTE !!!)
1. Manuseie a placa com cuidado.
2. Certifique-se de realizar o monitoramento serial.
