# TP de Synthèse – Autoradio  

##  Réalisé par:
- **AFIF Ikram**  
- **FAKHRI Menjli**

## Objectif du TP

L’objectif de ce TP est de configurer et d’exploiter différents périphériques de la carte STM32 NUCLEO-L476RG afin de construire un mini-système d’autoradio.
L’objectif final est d’acquérir une maîtrise de la configuration matérielle et de l’intégration logicielle dans un système embarqué complet.

---

# 1- Démarrage du projet

## 🔹 1.1 Création du projet (STM32CubeIDE)

Un projet a été généré pour la carte **NUCLEO-L476RG**, avec configuration de base et périphériques essentiels activés.  
Le BSP n’a pas été utilisé, conformément aux consignes.

## 🔹 1.2 Test de la LED LD2 (PA5)

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



## 🔹 1.3 Test UART2 → Affichage Terminal

Test d’envoi d’une chaîne via l’USART2 :

HAL_UART_Transmit(&huart2, (uint8_t*)"Hello\r\n", 7, HAL_MAX_DELAY);

## 🔹 1.4 Activation de printf

Redirection de l’affichage vers l’UART2 :

int __io_putchar(int chr)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)&chr, 1, HAL_MAX_DELAY);
    return chr;
}

## 🔹 1.5 Activation FreeRTOS (CMSIS V1)

FreeRTOS a été activé afin d’organiser le projet autour de tâches concurrentes.

2️⃣ Mise en place du Shell
💬 2.1 Shell exécuté dans une tâche FreeRTOS

Création de la tâche dédiée :

xTaskCreate(ShellTask, "shell", 256, NULL, 1, &h_shell_task);

void ShellTask(void *argument)
{
    shell_init();
    shell_run();
}

⚡ 2.2 Shell en interruption + sémaphore (Version finale)

Ceci est la version fonctionnelle et utilisée dans le TP.

📥 Lecture UART (uart_read)
static char uart_read() {
    HAL_UART_Receive_IT(&huart2, &rxbuffer, 1);
    xSemaphoreTake(uartRxSemaphore, HAL_MAX_DELAY);
    return (char)rxbuffer;
}

🔔 Callback d’interruption UART
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

🔑 Création du sémaphore
uartRxSemaphore = xSemaphoreCreateBinary();

3️⃣ Commandes du Shell
🟢 Ajout des commandes
shell_add('l', sh_led, "Toggle LED");
shell_add('b', sh_blink, "Blink LED");

## 🔸 Commande l : clignotement LED
int sh_led(int argc, char **argv)
{
    printf("toggle led\r\n");
    for (int i = 0; i < 10; i++)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(800);
    }
    return 0;
}

## 🔸 Commande b : allumer la LED
int sh_blink(int argc, char **argv)
{
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, 1);
    printf("Blink done\r\n");
    return 0;
}

4️⃣ Résultat dans le terminal
===== Monsieur Shell v0.2 =====
h
h : Help
l
toggle led
b
Blink done

