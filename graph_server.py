#!/usr/bin/env python3

import concurrent.futures
import logging
import socket
import struct
import subprocess
import tempfile

# Configurazione del server
HOST = "127.0.0.1"
PORT = 56181

# Configurazione del logging
logging.basicConfig(filename="server.log",
                    level=logging.DEBUG, datefmt='%H:%M:%S',
                    format='%(asctime)s - %(levelname)s - %(message)s')


def setup_server(host=HOST, port=PORT):
    """Configura il server e accetta connessioni."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server_socket.bind((host, port))
        server_socket.listen()
        logging.info(f"Server in ascolto su {host}:{port}")

        with concurrent.futures.ThreadPoolExecutor() as executor:
            while True:
                logging.info("In attesa di un client...")
                conn, addr = server_socket.accept()
                logging.info(f"Connessione accettata da {addr}")
                executor.submit(handle_client, conn, addr)


def handle_client(connection, address):
    """Gestisce la connessione con un client."""
    try:
        with tempfile.NamedTemporaryFile(mode='w', suffix=".mtx", delete=False) as temp_file:
            num_discarded = 0
            buffer = []

            N, A = receive_initial_data(connection)
            temp_file.write(f"{N} {N} {A}\n")

            num_discarded = process_edges(connection, A, N, buffer, temp_file)

            if buffer:
                temp_file.writelines(buffer)

            temp_file.seek(0)
            send_pagerank_results(connection, temp_file.name)

            logging.info(f"Nodi del grafo: {N}, Scartati: {num_discarded}, Validi: {A - num_discarded}")
            logging.info(f"File temporaneo: {temp_file.name}")
    finally:
        connection.close()


def receive_initial_data(connection):
    """Riceve i dati iniziali dal client."""
    data = connection.recv(8)
    if len(data) != 8:
        raise ValueError("Dati iniziali non validi ricevuti dal client")
    N, A = struct.unpack("!2i", data)
    return N, A


def process_edges(connection, num_edges, num_nodes, buffer, temp_file):
    """Processa gli archi ricevuti dal client."""
    num_discarded = 0
    for _ in range(num_edges):
        data = connection.recv(8)
        if len(data) != 8:
            logging.warning("Dati insufficienti ricevuti per un arco")
            continue

        From, To = struct.unpack("!2i", data)
        if not (0 < From <= num_nodes and 0 < To <= num_nodes):
            num_discarded += 1
            continue

        buffer.append(f"{From} {To}\n")
        if len(buffer) >= 10:
            temp_file.writelines(buffer)
            buffer.clear()

    return num_discarded


def send_pagerank_results(connection, temp_file_name):
    """Esegue il calcolo di PageRank e invia i risultati al client."""
    command = ['./pagerank', temp_file_name]
    result = subprocess.run(command, capture_output=True)

    if result.returncode != 0:
        send_data(connection, result.returncode, result.stderr)
    else:
        send_data(connection, 0, result.stdout)


def send_data(connection, return_code, data):
    """Invia il codice di ritorno e i dati al client."""
    connection.sendall(struct.pack("!i", return_code))
    connection.sendall(struct.pack("!i", len(data)))
    connection.sendall(data)


if __name__ == "__main__":
    setup_server()
