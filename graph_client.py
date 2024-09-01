#!/usr/bin/env python3

import socket
import struct
import sys
import threading

# Configurazione del server
HOST = "127.0.0.1"
PORT = 56181


def main(file_list):
    """Avvia i thread per elaborare ciascun file."""
    threads = []
    for filename in file_list:
        thread = threading.Thread(target=process_file, args=(filename,))
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()


def process_file(filename):
    """Elabora un singolo file e comunica con il server."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((HOST, PORT))
        send_file_content(sock, filename)
        receive_response(sock, filename)


def send_file_content(sock, filename):
    """Invia il contenuto del file al server."""
    with open(filename, "r") as file:
        for line in file:
            if line.startswith('%'):
                continue
            formatted_line = line.split()
            if len(formatted_line) == 3:
                sock.sendall(struct.pack("!2i", int(formatted_line[0]), int(formatted_line[2])))
            elif len(formatted_line) == 2:
                sock.sendall(struct.pack("!2i", int(formatted_line[0]), int(formatted_line[1])))


def receive_response(sock, filename):
    """Riceve la risposta dal server e la stampa."""
    return_code = struct.unpack("!i", sock.recv(4))[0]
    message_length = struct.unpack("!i", sock.recv(4))[0]
    message = sock.recv(message_length).decode()

    print(f"{filename} Exit code: {return_code}")
    for line in message.split('\n'):
        if line:
            print(f"{filename} {line}")

    print(f"{filename} Bye")


if __name__ == "__main__":
    if len(sys.argv) >= 2:
        main(sys.argv[1:])
    else:
        print(f"Uso: {sys.argv[0]} file1 ... fileN\n")
