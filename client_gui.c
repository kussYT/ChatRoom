#include <gtk/gtk.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#define LENGTH 2048

GtkWidget *chat_window;
GtkWidget *text_view;
GtkWidget *entry;
GtkTextBuffer *buffer;
int sockfd;
char name[32];

void send_msg_handler(GtkWidget *widget, gpointer data) {
    const gchar *message = gtk_entry_get_text(GTK_ENTRY(entry));
    char send_buffer[LENGTH + 32];
    char display_buffer[LENGTH + 32];  // Buffer séparé pour l'affichage

    if (strcmp(message, "exit") == 0) {
        gtk_main_quit(); // Quitter l'application
    } else {
        // Préparer le message à envoyer
        sprintf(send_buffer, "%s: %s\n", name, message);
        send(sockfd, send_buffer, strlen(send_buffer), 0);

        // Préparer le message à afficher
        sprintf(display_buffer, "You: %s\n", message);

        // Afficher le message envoyé dans le GtkTextView
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert(buffer, &end, display_buffer, -1); // Insérer le message envoyé dans le buffer
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &end, 0.0, FALSE, 0.0, 0.0); // Défilement automatique

        gtk_widget_show_all(chat_window); // Mise à jour de la fenêtre
    }

    gtk_entry_set_text(GTK_ENTRY(entry), ""); // Effacer le champ d'entrée
}

void recv_msg_handler() {
    char message[LENGTH];
    while (1) {
        int receive = recv(sockfd, message, LENGTH, 0);
        if (receive > 0) {
            GtkTextIter end;
            gtk_text_buffer_get_end_iter(buffer, &end); // Obtenir l'itérateur à la fin du texte
            gtk_text_buffer_insert(buffer, &end, message, -1); // Insérer le message reçu
            gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &end, 0.0, FALSE, 0.0, 0.0); // Scroll vers le bas
            gtk_widget_show_all(chat_window); // Mise à jour de la fenêtre
        }
        memset(message, 0, sizeof(message)); // Réinitialiser le message
    }
}

void *recv_thread_func(void *arg) {
    recv_msg_handler();
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    gtk_init(&argc, &argv);

    // Setup network (similar to original code)
    char *ip = "127.0.0.1";
    int port = atoi(argv[1]);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    int err = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (err == -1) {
        printf("ERROR: connect\n");
        return EXIT_FAILURE;
    }

    // Ask for user name
    printf("Please enter your name: ");
    fgets(name, 32, stdin);
    name[strlen(name) - 1] = '\0'; // Remove trailing newline
    send(sockfd, name, 32, 0);

    // Create chat window
    chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(chat_window), "My Chat Room");
    gtk_window_set_default_size(GTK_WINDOW(chat_window), 400, 300);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(chat_window), vbox);

    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_box_pack_start(GTK_BOX(vbox), text_view, TRUE, TRUE, 0);

    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    GtkWidget *send_button = gtk_button_new_with_label("Send");
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_msg_handler), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), send_button, FALSE, FALSE, 0);

    g_signal_connect(chat_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, recv_thread_func, NULL);

    gtk_widget_show_all(chat_window);
    gtk_main();

    close(sockfd);
    return EXIT_SUCCESS;
}
