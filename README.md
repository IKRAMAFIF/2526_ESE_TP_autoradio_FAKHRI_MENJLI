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

