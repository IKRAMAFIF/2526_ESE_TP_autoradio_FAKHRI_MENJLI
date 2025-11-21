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

![Test Affichage](Test_AffichageTeraTerm.jpeg)


##  1.5 Activation FreeRTOS (CMSIS V1)
FreeRTOS a été activé depuis STM32CubeMX en utilisant l’interface **CMSIS V1**.  
Cette étape permet d’organiser l’application autour de plusieurs tâches gérées par l’ordonnanceur FreeRTOS.

Voici la configuration utilisée :
![Activation FreeRTOS](Activation%20FreeRTOS.png)

##  1.6 Mise en place d’un Shell fonctionnel
Un shell interactif a été ajouté afin de permettre l’envoi de commandes via le terminal (Tera Term).  
Ce shell fonctionne sur l’USART2 et permet d’exécuter différentes actions.

### 1.6.a Shell exécuté dans une tâche FreeRTOS

Dans un premier temps, le shell a été exécuté directement dans une tâche FreeRTOS dédiée.  
Cette approche permet au shell de tourner en continu, en lisant les caractères reçus sur l’USART2 via `shell_run()`.

La tâche a été créée avec la fonction `xTaskCreate` :

```c
xTaskCreate(
    ShellTask,
    "ShellTask",
    512,
    NULL,
    1,
    NULL
);
```
![Shell 1a](Shell1a.jpeg)

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
![Shell 2a](Shell2a.jpeg)

- **Résultat obtenu dans Tera Term:**

![Shell 3a](Shell3a.jpeg)


###   1.6.b  – Fonctionnement du Shell en interruption + sémaphore FreeRTOS

Dans cette version, la réception UART ne se fait plus en mode bloquant, mais via une **interruption**.  
Chaque caractère reçu déclenche une **IT UART**, qui réveille la tâche du shell grâce à un **sémaphore binaire**.

Ce mécanisme nous permet d’éviter de bloquer la tâche, d’avoir une meilleure réactivité, et d’être compatible avec FreeRTOS.

- À chaque appel, la fonction déclenche une réception IT et attend le sémaphore :

```c
static char uart_read() {

    HAL_UART_Receive_IT(&huart2, &rxbuffer, 1);
    xSemaphoreTake(uartRxSemaphore, HAL_MAX_DELAY);

    return (char)rxbuffer;
}
```
![Shell 1b](Shell1b.jpeg)

- L’interruption donne le sémaphore pour réveiller la tâche Shell :

```
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        xSemaphoreGiveFromISR(uartRxSemaphore, &xHigherPriorityTaskWoken);
        HAL_UART_Receive_IT(&huart2, &rxbuffer, 1);

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}
```
![Shell 2b](Shell2b.jpeg)

- Création du sémaphore + lancement de la tâche :
  
```
  if (xTaskCreate(ShellTask,"shell", 256, NULL,1, &h_shell_task)!=pdPASS)
{
    printf("error creation task shell\r\n");
    Error_Handler();
}

uartRxSemaphore = xSemaphoreCreateBinary();
if (uartRxSemaphore == NULL)
{
    Error_Handler();
}

vTaskStartScheduler();
```
![Shell 3b](Shell3b.jpeg)

- Résultat sur Tera Term (réception OK via interruptions)

![Shell 4b](Shell4b.jpeg)

###   1.6.c  –Fonctionnement du Shell avec un driver
Dans cette version, le shell n’utilise plus directement les fonctions UART ni le sémaphore.  
Toutes les opérations d’entrée/sortie sont encapsulées dans une **structure driver**, ce qui rend le shell plus modulable et indépendant du matériel.

Le shell est maintenant contrôlé via les deux pointeurs de fonctions :
- `drv.receive` pour lire un caractère  
- `drv.transmit` pour envoyer une chaîne

- Initialisation du driver:

- Commande associée : fonction()
 Cette fonction envoie un message via le driver, sans utiliser printf :


- Résultat obtenu dans Tera Term
Le shell utilise bien le driver pour transmettre la réponse :




