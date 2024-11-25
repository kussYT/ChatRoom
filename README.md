# ChatRoom  

## Description  

Le projet **ChatRoom** est une application de messagerie en temps réel permettant à plusieurs utilisateurs de se connecter à un serveur et de discuter entre eux. Les messages envoyés par chaque utilisateur sont diffusés à tous les autres participants connectés à la même session.  

L'interface utilisateur côté client utilise GTK+ 3 pour offrir une interface graphique conviviale, tandis que le serveur gère la communication entre les clients à l'aide de sockets et de threads en C.  

Ce projet est un excellent exemple de l'utilisation de concepts tels que :  
- Programmation multithreadée avec `pthread`  
- Sockets réseau en C (`TCP/IP`)  
- Interfaces graphiques avec `GTK+`  

## Fonctionnalités  

- Gestion de plusieurs utilisateurs simultanément (jusqu'à une limite prédéfinie).  
- Interface graphique pour les utilisateurs côté client.  
- Notifications lorsque des utilisateurs rejoignent ou quittent la chatroom.  
- Envoi et réception de messages en temps réel.  

## Prérequis  

Avant de lancer le projet, assurez-vous d'avoir installé les éléments suivants :  

1. **Compilateur GCC** : Pour compiler les fichiers C.  
2. **GTK+ 3** : Nécessaire pour l'interface graphique côté client. Installez-le sur votre système :  
   ```bash
   sudo apt-get install libgtk-3-dev