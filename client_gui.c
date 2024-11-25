#include <gtk/gtk.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

#define LENGTH 2048

// Global variables for GTK widgets and network communication
GtkWidget *chat_window;
GtkWidget *text_view;
GtkWidget *entry;
GtkTextBuffer *buffer;
int sockfd;
char name[32];

// Function to handle sending messages
void send_msg_handler(GtkWidget *widget, gpointer data) {
    const gchar *message = gtk_entry_get_text(GTK_ENTRY(entry)); // Get text from the entry field
    char send_buffer[LENGTH + 32]; // Buffer for the message to send
    char display_buffer[LENGTH + 32]; // Separate buffer for displaying the message

    if (strcmp(message, "exit") == 0) {
        gtk_main_quit(); // Exit the application
    } else {
        // Prepare the message to send
        sprintf(send_buffer, "%s: %s\n", name, message);
        send(sockfd, send_buffer, strlen(send_buffer), 0);

        // Prepare the message for display in the chat window
        sprintf(display_buffer, "You: %s\n", message);

        // Add the sent message to the GtkTextView
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert(buffer, &end, display_buffer, -1); // Insert message into the buffer
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &end, 0.0, FALSE, 0.0, 0.0); // Auto-scroll

        gtk_widget_show_all(chat_window); // Update the chat window
    }

    gtk_entry_set_text(GTK_ENTRY(entry), ""); // Clear the entry field
}

// Function to handle receiving messages
void recv_msg_handler() {
    char message[LENGTH];
    while (1) {
        // Receive message from the server
        int receive = recv(sockfd, message, LENGTH, 0);
        if (receive > 0) {
            GtkTextIter end;
            gtk_text_buffer_get_end_iter(buffer, &end); // Get iterator to the end of the text
            gtk_text_buffer_insert(buffer, &end, message, -1); // Insert received message into the buffer
            gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &end, 0.0, FALSE, 0.0, 0.0); // Auto-scroll
            gtk_widget_show_all(chat_window); // Update the chat window
        }
        memset(message, 0, sizeof(message)); // Clear the message buffer
    }
}

// Thread function to handle receiving messages
void *recv_thread_func(void *arg) {
    recv_msg_handler(); // Call the message receiving function
    return NULL;
}

// Main function
int main(int argc, char **argv) {
    // Check for correct usage
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    gtk_init(&argc, &argv); // Initialize GTK

    // Setup network communication
    char *ip = "127.0.0.1"; // Server IP
    int port = atoi(argv[1]); // Convert port argument to integer

    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Create socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip); // Set server IP
    server_addr.sin_port = htons(port); // Set server port

    // Connect to the server
    int err = connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if (err == -1) {
        printf("ERROR: connect\n");
        return EXIT_FAILURE;
    }

    // Prompt user for their name
    printf("Please enter your name: ");
    fgets(name, 32, stdin);
    name[strlen(name) - 1] = '\0'; // Remove trailing newline
    send(sockfd, name, 32, 0); // Send name to the server

    // Create chat window
    chat_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(chat_window), "My Chat Room");
    gtk_window_set_default_size(GTK_WINDOW(chat_window), 400, 300);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); // Vertical box layout
    gtk_container_add(GTK_CONTAINER(chat_window), vbox);

    // Create text view for displaying chat messages
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE); // Make it read-only
    buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    gtk_box_pack_start(GTK_BOX(vbox), text_view, TRUE, TRUE, 0);

    // Create entry field for typing messages
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);

    // Create send button
    GtkWidget *send_button = gtk_button_new_with_label("Send");
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_msg_handler), NULL); // Connect send button to handler
    gtk_box_pack_start(GTK_BOX(vbox), send_button, FALSE, FALSE, 0);

    // Connect the close event to quit the application
    g_signal_connect(chat_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Create a thread for receiving messages
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, recv_thread_func, NULL);

    // Show all widgets in the chat window
    gtk_widget_show_all(chat_window);

    gtk_main(); // Start the GTK main loop

    close(sockfd); // Close the socket on exit
    return EXIT_SUCCESS;
}
