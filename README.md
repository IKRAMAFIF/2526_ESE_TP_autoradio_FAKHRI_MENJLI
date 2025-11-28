# TP de Synthèse – Autoradio  

##  Réalisé par:
- **AFIF Ikram**  
- **FAKHRI Menjli**

## Objectif du TP

L’objectif de ce TP est de configurer et d’exploiter différents périphériques de la carte STM32 NUCLEO-L476RG afin de construire un mini-système d’autoradio.
L’objectif final est d’acquérir une maîtrise de la configuration matérielle et de l’intégration logicielle dans un système embarqué complet.

---

# 1- Démarrage du projet

##  1.1 Création du projet (STM32CubeIDE)

Un projet a été généré pour la carte **NUCLEO-L476RG**, avec configuration de base et périphériques essentiels activés.  
Le BSP n’a pas été utilisé, conformément aux consignes.

##  1.2 Test de la LED LD2 (PA5)

Nous avons validé la configuration GPIO en effectuant un clignotement simple :

```c
int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
HAL_Delay(500);

```

##  1.3 Test UART2 

Pour tester la communication série entre la carte NUCLEO-L476RG et le PC via la STLink, nous avons envoyé régulièrement une chaîne de caractères sur l’USART2.
```
while (1)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)"Hello\r\n", 7, HAL_MAX_DELAY);
    HAL_Delay(500);
}
```
##  1.4 Activation de printf

Pour permettre l’utilisation de `printf` via l’USART2, nous avons redirigé la sortie standard vers l’UART.
Ajout de la fonction suivante :
```
int __io_putchar(int chr)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)&chr, 1, HAL_MAX_DELAY);
    return chr;
}
```
Test d’affichage :

```
printf("Test printf sur USART2 !\r\n");
```
L’affichage des messages dans le terminal série confirme le bon fonctionnement de l’USART2, comme présenté ci-dessous:

![Test Affichage](assets/Test_AffichageTeraTerm.jpeg)


##  1.5 Activation FreeRTOS (CMSIS V1)
FreeRTOS a été activé depuis STM32CubeMX en utilisant l’interface **CMSIS V1**.  
Cette étape permet d’organiser l’application autour de plusieurs tâches gérées par l’ordonnanceur FreeRTOS.

Voici la configuration utilisée :
![Activation FreeRTOS](assets/Activation%20FreeRTOS.png)

##  1.6 Mise en place d’un Shell fonctionnel
Un shell interactif a été ajouté afin de permettre l’envoi de commandes via le terminal (Tera Term).  
Ce shell fonctionne sur l’USART2 et permet d’exécuter différentes actions.

### 1.6.a Shell exécuté dans une tâche FreeRTOS

Dans un premier temps, le shell a été exécuté directement dans une tâche FreeRTOS dédiée.  
Cette approche permet au shell de tourner en continu, en lisant les caractères reçus sur l’USART2 via `shell_run()`.

La tâche a été créée avec la fonction `xTaskCreate` :

![Shell 1a](assets/Shell1a.jpeg)

- **Fonctionnement de la tâche ShellTask**
 ```
void ShellTask(void *pvParameters)
{
    shell_init();

    shell_add('l', sh_led, "Toggle LED");
    shell_add('b', sh_blink, "Blink LED");

    shell_run();

    vTaskDelete(NULL);
}
```
- **Commande (Toggle LED)**
```
int sh_led(int argc, char **argv)
{
    int i;
    printf("toggle led\r\n");
    for (i = 0; i < 10; i++)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(800);
    }
    return 0;
}
```
- **Commande (Blink LED)**
```
int sh_blink(int argc, char **argv)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 1);
    printf("Blink done\r\n");
    return 0;
}
```
![Shell 2a](assets/Shell2a.jpeg)

- **Résultat obtenu dans Tera Term:**

![Shell 3a](assets/Shell3a.jpeg)


### 1.6.b  – Fonctionnement du Shell en interruption + sémaphore FreeRTOS

Dans cette version, la réception UART ne se fait plus en mode bloquant, mais via une **interruption**.  
Chaque caractère reçu déclenche une **IT UART**, qui réveille la tâche du shell grâce à un **sémaphore binaire**.

Ce mécanisme nous permet d’éviter de bloquer la tâche, d’avoir une meilleure réactivité, et d’être compatible avec FreeRTOS.

- À chaque appel, la fonction déclenche une réception IT et attend le sémaphore :

![Shell 1b](assets/Shell1b.jpeg)

- L’interruption donne le sémaphore pour réveiller la tâche Shell :

![Shell 2b](assets/Shell2b.jpeg)

- Création du sémaphore + lancement de la tâche :
  
![Shell 3b](assets/Shell3b.jpeg)

- Résultat sur Tera Term (réception OK via interruptions)

![Shell 4b](assets/Shell4b.jpeg)

###  1.6.c  –Fonctionnement du Shell avec un driver
Dans cette version, le shell n’utilise plus directement les fonctions UART ni le sémaphore.  
Toutes les opérations d’entrée/sortie sont encapsulées dans une **structure driver**, ce qui rend le shell plus modulable et indépendant du matériel.

Le shell est maintenant contrôlé via les deux pointeurs de fonctions :
- `drv.receive` pour lire un caractère  
- `drv.transmit` pour envoyer une chaîne

- Initialisation du driver:

![Shell 1c](assets/Shell1c.jpeg)

- Commande associée : fonction()
 Cette fonction envoie un message via le driver, sans utiliser printf :

![Shell 2c](assets/Shell2c.jpeg)

- Résultat obtenu dans Tera Term
Le shell utilise bien le driver pour transmettre la réponse :

![Shell 3c](assets/Shell3c.jpeg)

---

# 2- Le GPIO Expander et le VU-Metre

## 2.1 Configuration

## 2.1 Configuration du GPIO Expander

### 2.1.1 Référence du GPIO Expander
Le composant utilisé dans ce TP est le **MCP23S17**, un GPIO Expander fonctionnant en **SPI**.

### 2.1.2 SPI utilisé sur le STM32
Dans notre configuration, le STM32 utilise l’interface **SPI3** pour communiquer avec le MCP23S17.

### 2.1.3 Paramètres SPI à configurer dans STM32CubeIDE
Les paramètres essentiels du SPI3 sont les suivants :
- **Mode** : Full-Duplex Master  
- **Frame Format** : Motorola  
- **Data Size** : 8 bits  
- **First Bit** : MSB First  
- **Clock Prescaler** : 2 (40 Mbit/s)  
- **NSS** : Software ou Hardware Disable  
- **Polarity / Phase (CPOL/CPHA)** : par défaut  
- **GPIO associés** :
  - SCK → PC10  
  - MISO → PC11  
  - MOSI → PC12  
  - CS → Broche manuelle (VU_nRESET ici)
  - 
### 2.1.4 Configuration effectuée
Voici la capture de configuration utilisée :

![Configuration SPI](assets/ConfigurationSPI.jpeg)

## 2.2 Tests

### 2.2.1 Faire clignoter une ou plusieurs LED

Pour tester le fonctionnement du GPIO Expander (MCP23S17), nous avons commencé par allumer et éteindre une LED connectée au module via SPI.

![Test 2a](assets/Test2a.jpeg)


### 2.2.2 Tester toutes les LED (chenillard)

Pour tester l’ensemble des sorties du GPIO Expander, nous avons implémenté un chenillard, c’est-à-dire une LED qui avance de bit en bit.

![Test 2b](assets/Test2b.jpeg)

![Test 2b](assets/Test2b.gif)

## 2.3 Driver

Dans cette partie, nous avons créé un **driver générique** permettant de contrôler les LED connectées au GPIO Expander **MCP23S17** via SPI.  
L'objectif est de séparer la logique applicative (shell) de la couche matérielle (SPI + MCP23S17).

### 2.3.1 Structure du driver

Le driver est implémenté à l’aide d’une structure contenant deux fonctions principales :
- écriture dans un registre du MCP23S17
- contrôle d’une LED (ON/OFF)

```c
typedef struct {
    void (*write)(uint8_t reg, uint8_t value);
    void (*set_pin)(char port, uint8_t pin, uint8_t state);
} led_driver_t;

led_driver_t drv_led;
```

### 2.3.1 Fonction shell pour allumer/éteindre n’importe quelle LED

Pour répondre à cette question, nous avons créé une fonction shell permettant de contrôler une LED le périphérique GPIO Expander MCP23S17 via une commande shell.

A 2 1 → LED A2 ON
A 1 0 → LED A1 OFF

![Driver](assets/Driver.jpeg)



## 3. Le CODEC Audio SGTL5000
### 3.1 Configuration préalables

Le CODEC SGTL5000 nécessite deux protocoles pour fonctionner :
**I2C → configuration des registres internes du CODEC**
**I2S (via SAI2) → transfert des échantillons audio (lecture & écriture)**
Toutes ces configurations sont réalisées dans STM32CubeIDE / CubeMX.

#### 3.1.1/2 Configuration de l’I2C
- Pins utilisées pour l’I2C

Sur notre carte STM32L476RG, l’I2C utilisé pour communiquer avec le CODEC passe par :
PB10 → I2C2_SCL
PB11 → I2C2_SDA

![I2C Pins](ActivationI2C.png)

 3.1.3 Configuration du SAI2 (I2S)

Le CODEC audio utilise I2S, fourni par le périphérique SAI2 du STM32.
Configuration du SAI A
Mode : Master with Master Clock Out
Protocole : I2S/PCM
Configuration du SAI B
Mode : Synchronous Slave
Protocole : I2S/PCM
 Capture SAI2 Config

![SAI2 Config](SAI2_Config.png)

 3.1.4 Affectation des broches SAI

Les broches doivent correspondre EXACTEMENT à celles du CODEC.

Signal SAI2	Broche STM32
FS_A	PB12
SCK_A	PB13
MCLK_A	PB14
SD_A	PB15
SD_B	PC12

 3.1.5 Configuration de la Clock – PLLSAI1 = 12.235294 MHz

Pour que le CODEC fonctionne, il doit recevoir une horloge MCLK précise.
Nous configurons donc PLLSAI1 pour générer : 12.235294 MHz pour SAI2
 Capture PLLSAI1

![PLLSAI Clock Config](PLLSAI_Config.png)

 3.1.6 Configuration des blocs SAI A et SAI B

Les blocs SAI sont configurés comme suit :

 SAI A

Mode : Master
Protocole : I2S
Fournit MCLK, SCK, FS, SD_A

 SAI B

Mode : Synchronous Slave

Protocole : I2S

Fournit SD_B

**Activation des interruptions**

Les interruptions du SAI sont ensuite activées pour gérer :

- La réception des samples audio
- L’envoi automatique via DMA











